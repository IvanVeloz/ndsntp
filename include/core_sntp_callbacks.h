/*
 * Copyright (C) 2024 Ivan Veloz.  All Rights Reserved.
 *
 * SPDX-License-Identifier: MIT
*/
#ifndef CORE_SNTP_CALLBACKS_H_
#define CORE_SNTP_CALLBACKS_H_

#include <stdbool.h>
#include <core_sntp_client.h>

bool sntpResolveDns(const SntpServerInfo_t * pServerAddr,
                            uint32_t * pIpV4Addr);

#endif  /* ifndef CORE_SNTP_CALLBACKS_H_ */