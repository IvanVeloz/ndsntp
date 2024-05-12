/*
 * Copyright (C) 2024 Ivan Veloz.  All Rights Reserved.
 *
 * SPDX-License-Identifier: MIT
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
    *pIpV4Addr = (uint32_t)addr.s_addr;
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
        LogWarn(("Could not get time from RTC. Continuing.\n"));
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
        LogWarn(("Could not get time from SNTP. Continuing.\n"));
    }
    struct timespec t = {
        .tv_sec  = (time_t)s,
        .tv_nsec = (time_t)(ms*1000),
    };
    clock_settime(CLOCK_REALTIME, &t);
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
    struct timeval tout = {.tv_sec = 0, .tv_usec = 0};
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(pNetworkContext->udpSocket, &fds);
     
    int r = select( pNetworkContext->udpSocket+1,
                    NULL, &fds, &fds, &tout);
    if(r < 0) {
        LogWarn(("Could not open UDP socket for writing. Aborting.\n"));
        LogWarn(("Errno was %i\n", errno));
        return r;
    }
    else if(r > 0) {
        struct sockaddr_in addri;
        addri.sin_family = AF_INET;
        addri.sin_port = htons(serverPort);
        addri.sin_addr.s_addr = htonl(serverAddr);
        
        r = sendto( pNetworkContext->udpSocket, pBuffer, bytesToSend, 0,
                    (const struct sockaddr *)(&addri), sizeof(addri));
    }
    else if(r == 0) {
        r = 0;
    }
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
    struct timeval tout = {.tv_sec = 0, .tv_usec = 0};
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(pNetworkContext->udpSocket, &fds);
     
    int r = select( pNetworkContext->udpSocket+1,
                    &fds, NULL, &fds, &tout);
    if(r < 0) {
        LogWarn(("Could not open UDP socket for reading. Aborting.\n"));
        LogWarn(("Errno was %i\n", errno));
        return r;
    }
    else if (r > 0) {
        struct sockaddr_in addri;
        addri.sin_family = AF_INET;
        addri.sin_port = htons(serverPort);
        addri.sin_addr.s_addr = htonl(serverAddr);
        int addrlen = sizeof(addri);
        r = recvfrom(   pNetworkContext->udpSocket, pBuffer,bytesToRecv, 0,
                        (struct sockaddr *)(&addri), &addrlen);
    }
    else if (r == 0) {
        r = 0;
    }
    return r;
}
