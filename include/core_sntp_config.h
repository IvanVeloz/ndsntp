/*
 * Copyright (C) 2024 Ivan Veloz.  All Rights Reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef CORE_SNTP_CONFIG_H_
#define CORE_SNTP_CONFIG_H_

#include <stdio.h>

#ifndef LogError
    #define LogError( message )     \
        iprintf("coreSNTP: ");      \
        iprintf("ERROR: ");         \
        iprintf message;            \
        iprintf("\n");
#endif

#ifndef LogWarn
    #define LogWarn( message )      \
        iprintf("coreSNTP: ");      \
        iprintf("WARN: ");          \
        iprintf message;            \
        iprintf("\n");
#endif

#ifndef LogInfo
    #define LogInfo( message )      \
        iprintf("coreSNTP: ");      \
        iprintf("info: ");          \
        iprintf message;            \
        iprintf("\n");
#endif

#ifndef LogDebug
    #define LogDebug( message )     \
        iprintf("coreSNTP: ");      \
        iprintf("debug: ");         \
        iprintf message;            \
        iprintf("\n");
#endif

#endif /* ifndef CORE_SNTP_CONFIG_DEFAULTS_H_ */