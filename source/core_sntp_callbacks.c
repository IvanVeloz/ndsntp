/*
 * Copyright (C) 2024 Ivan Veloz.  All Rights Reserved.
 *
 * SPDX-License-Identifier: MIT
*/
#include <nds/system.h>         /* Real time clock */
#include <sys/socket.h>         /* Network sockets */
#include <netdb.h>              /* DNS lookups */
#include <time.h>               /* Time and calendar */
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

    *pIpV4Addr = (uint32_t)ntphost->h_addr_list[0];
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