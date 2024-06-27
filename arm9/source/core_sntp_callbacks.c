/*
 * Copyright (C) 2024 Ivan Veloz.  All Rights Reserved.
 *
 * SPDX-License-Identifier: MIT
 * SPDX-FileContributor: Ivan Veloz, 2024
 */
#include <nds/system.h>         /* Real time clock */
#include <sys/socket.h>         /* Network sockets */
#include <sys/select.h>         /* fd_set type and macros for select() */
#include <netinet/in.h>         /* socketaddr_in */
#include <netdb.h>              /* DNS lookups */
#include <time.h>               /* Time and calendar */
#include <errno.h>
#include <core_sntp_client.h>
#include "core_sntp_callbacks.h"
#include "core_sntp_config_defaults.h"


/** 
 * @brief Resolves the time server domain-name to an IPv4 address. The coreSNTP 
 * client library will call this function every time the server is used. Caching
 * is not implemented, it is possible to implement.
 * 
 * Corresponds to SntpResolveDns_t callback.
 */
bool sntpResolveDns(const SntpServerInfo_t * pServerAddr,
                            uint32_t * pIpV4Addr) 
{
   	struct hostent * ntphost = gethostbyname(pServerAddr->pServerName);

    if(ntphost == NULL)                 return false;
	if(ntphost->h_addrtype != AF_INET)  return false;
	if(ntphost->h_length != 4)          return false;

    struct in_addr addr = *(struct in_addr *)ntphost->h_addr_list[0];
    *pIpV4Addr = htonl((uint32_t)addr.s_addr);
    return true;
}

/**
 * @brief Obtains current system time from the NDS BIOS and converts it into an
 * SNTP timestamp format. Corresponds to SntpGetTime_t callback.
 * 
 * To reduce complexity, we make a few assumptions:
 * 1. The adjustments for leap seconds are being handled by the standard C 
 * library (newlibc in the case of DevKitPro, picolibc in the case of BlocksDS).
 * Consult the corresponding documentation.
 * 2. No adjustments have been made to account for the delay in getting the
 * time from the RTC (or the function itself).
 * 3. Accuracy better than 1 second is not necessary. I would like to improve
 * this down to 10ms eventually, which is the highest date resolution of the FAT 
 * filesystem.
 * 
 * And we make the following assertions:
 * 1. There were no leap seconds between the NTP epoch (1900-01-01T00:00:00Z) 
 * and Unix epoch (1970-01-01T00:00:00Z). The number of seconds between the 
 * NTP epoch and the Unix epoch is 2208988800L according to RFC 868.
 * 
 */
void sntpGetTime(SntpTimestamp_t * pCurrentTime)
{
    time_t unixTime = time(NULL);
    if(unixTime == (time_t)(-1)) {
        LogWarn(("Could not get time from RTC. Continuing."));
    }
    pCurrentTime->seconds = unixTime + 2208988800L;
}

/**
 * @brief Obtains UTC time from an SNTP timestamp and stores it in the NDS RTC.
 * 
 * To reduce complexity, we compromised and we are using the
 * `Sntp_ConverToUnixTime` function, which is NOT Y2K28-proof. This was a
 * deliberate decition on the part of the coreSNTP team. In the near future, I
 * would like to rewrite this `sntpSetTime` function to be Y2K38-proof. This
 * probably means handling our own conversion or finding a library to do so.
 * 
 * We also made the following assumptions:
 * 1. No adjustments have been made to account for the delay in getting the
 * time from the RTC (or the function itself).
 * 2. Accuracy better than 1 second is not necessary. I would like to improve
 * this down to 10ms eventually, which is the highest date resolution of the FAT 
 * filesystem.
 */
void sntpSetTime(   const SntpServerInfo_t * pTimeServer, 
                    const SntpTimestamp_t * pServerTime,
                    int64_t clockOffsetMs,
                    SntpLeapSecondInfo_t leapSecondInfo )
{
    uint32_t s, ms;
    SntpStatus_t status = Sntp_ConvertToUnixTime(pServerTime, &s, &ms);
    if(status != SntpSuccess) {
        LogWarn(("Could not get time from SNTP. Skipping time setting."));
        return;
    }
    struct timespec t = {
        .tv_sec  = (time_t)s,
        .tv_nsec = (time_t)(ms*1000),
    };

    struct tm ts;
    ts = *localtime(&t.tv_sec);

    rtcTimeAndDate rtctime = {
        .year = ts.tm_year-100,        // - works until 2099. % works forever
        .month = ts.tm_mon+1,
        .day = ts.tm_mday,
        .weekday = ts.tm_wday,
        .day = ts.tm_mday,
        .hours = ts.tm_hour,
        .minutes = ts.tm_min,
        .seconds = ts.tm_sec
    };
    fifoSendDatamsg(FIFO_USER_01, sizeof(rtctime), (void *)&rtctime);
    LogInfo(("RTC set to %lli",t.tv_sec));
}

/**
 * 
 */
int32_t sntpUdpSend(NetworkContext_t * pNetworkContext,
                    uint32_t serverAddr,
                    uint16_t serverPort,
                    const void * pBuffer,
                    uint16_t bytesToSend)
{
    struct sockaddr_in addri;
    addri.sin_family = AF_INET;
    addri.sin_port = htons(serverPort);
    addri.sin_addr.s_addr = htonl(serverAddr);
    connect(pNetworkContext->udpSocket,
            (struct sockaddr*)&addri,
            sizeof(addri));
    /*
     * NOTE: dswifi select() is not implemented for UDP, it seems.
     * TODO: investigate further, contribute fix if necessary.
     * ```
     * fd_set fds;
     * FD_ZERO(&fds);
     * FD_SET(pNetworkContext->udpSocket, &fds);
     * struct timeval tout = {.tv_sec = 0, .tv_usec = 100};
     * int r = select( pNetworkContext->udpSocket+1,
     *                 NULL, &fds, &fds, &tout);
     * ```
     */
    int r = 1;
    if(r < 0) {
        LogError(("Could not poll UDP socket for writing. Aborting."));
        LogError(("Errno was %i", errno));
        goto cleanup;
    }
    else if(r > 0) {

        r = sendto( pNetworkContext->udpSocket, pBuffer, bytesToSend, 0,
                    (const struct sockaddr *)(&addri), sizeof(addri));
    }
    else if(r == 0) {
        // Timed out. This is normal.
        goto cleanup;
    }
    cleanup:
  	shutdown(pNetworkContext->udpSocket,0);
    return r;
}

/**
 * 
 */
int32_t sntpUdpRecv(NetworkContext_t * pNetworkContext,
                    uint32_t serverAddr,
                    uint16_t serverPort,
                    void * pBuffer,
                    uint16_t bytesToRecv)
{
    struct sockaddr_in addri;
    addri.sin_family = AF_INET;
    addri.sin_port = htons(serverPort);
    addri.sin_addr.s_addr = htonl(serverAddr);
    connect(pNetworkContext->udpSocket,
            (struct sockaddr*)&addri,
            sizeof(addri));

    struct timeval tout = {.tv_sec = 0, .tv_usec = 100};
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(pNetworkContext->udpSocket, &fds);
     
    int r = select( pNetworkContext->udpSocket+1,
                    &fds, NULL, &fds, &tout);
    if(r < 0) {
        LogWarn(("Could not open UDP socket for reading. Aborting."));
        LogWarn(("Errno was %i", errno));
        goto cleanup;
    }
    else if (r > 0) {
        int addrlen = sizeof(addri);
        r = recvfrom(   pNetworkContext->udpSocket, pBuffer,bytesToRecv, 0,
                        (struct sockaddr *)(&addri), &addrlen);
    }
    else if (r == 0) {
        // Timed out. This is normal.
        goto cleanup;
    }
    cleanup:
   	shutdown(pNetworkContext->udpSocket,0);
    return r;
}
