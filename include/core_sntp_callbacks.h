/*
 * Copyright (C) 2024 Ivan Veloz.  All Rights Reserved.
 *
 * SPDX-License-Identifier: MIT
*/
#ifndef CORE_SNTP_CALLBACKS_H_
#define CORE_SNTP_CALLBACKS_H_

#include <stdbool.h>
#include <core_sntp_client.h>
#include <sys/socket.h>

struct NetworkContext
{
    int udpSocket;
};

bool sntpResolveDns(const SntpServerInfo_t * pServerAddr,
                            uint32_t * pIpV4Addr);

void sntpGetTime(SntpTimestamp_t * pCurrentTime);

void sntpSetTime(   const SntpServerInfo_t * pTimeServer, 
                    const SntpTimestamp_t * pServerTime,
                    int64_t clockOffsetMs,
                    SntpLeapSecondInfo_t leapSecondInfo);

int32_t sntpUdpSend(NetworkContext_t * pNetworkContext,
                    uint32_t serverAddr,
                    uint16_t serverPort,
                    const void * pBuffer,
                    uint16_t bytesToSend);

int32_t sntpUdpRecv(NetworkContext_t * pNetworkContext,
                    uint32_t serverAddr,
                    uint16_t serverPort,
                    void * pBuffer,
                    uint16_t bytesToRecv);

#endif  /* ifndef CORE_SNTP_CALLBACKS_H_ */
