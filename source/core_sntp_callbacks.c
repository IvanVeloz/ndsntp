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
 * To reduce complexity, several assumptions have been made.
 * 1. The number of seconds since the NTP epoch (00:00 on 01/01/1900) is assumed
 * to be
 * 
 */
void sntpGetTime(SntpTimestamp_t * pCurrentTime)
{
    
}