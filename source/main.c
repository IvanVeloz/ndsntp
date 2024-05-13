/*
 * Copyright (C) 2024 Ivan Veloz.  All Rights Reserved.
 *
 * SPDX-License-Identifier: MIT
*/

#define CORE_SNTP_LOG_LEVEL 6

#include <nds.h>
#include <unistd.h>
#include <dswifi9.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <assert.h>
#include <core_sntp_client.h>
#include "core_sntp_callbacks.h"
#include "core_sntp_config.h"

/* --- START OF coreSNTP EXAMPLE --- */
/* This section is Copyright (c) Amazon Inc. or its affiliates. 
 * Distribuited under the MIT license */

/* Configuration constants for the example SNTP client. */
 
/* Following Time Servers are used for illustrating the usage of library API.
 * The library can be configured to use ANY time server, whether publicly available
 * time service like NTP Pool or a privately owned NTP server. */
#define TEST_TIME_SERVER_1                      "0.pool.ntp.org"
#define TEST_TIME_SERVER_2                      "1.pool.ntp.org"
 
#define SERVER_RESPONSE_TIMEOUT_MS              3000
#define TIME_REQUEST_SEND_WAIT_TIME_MS          2000
#define TIME_REQUEST_RECEIVE_WAIT_TIME_MS       1000
 
#define SYSTEM_CLOCK_FREQUENCY_TOLERANCE_PPM    500
#define SYSTEM_CLOCK_DESIRED_ACCURACY_MS        300

/* --- END OF coreSNTP EXAMPLE ---*/


char * ntpurl = "us.pool.ntp.org";

int main(void) {

	struct in_addr ip, gateway, mask, dns1, dns2;

	consoleDemoInit();

	iprintf("Connecting via WFC data\n");

	if(!Wifi_InitDefault(WFC_CONNECT)) {
		iprintf("Connection failed.\n");
		goto idle;
	}

	iprintf("Connected.\n");

	ip = Wifi_GetIPInfo(&gateway, &mask, &dns1, &dns2);
	
	iprintf("ip     : %s\n", inet_ntoa(ip) );
	iprintf("gateway: %s\n", inet_ntoa(gateway) );
	iprintf("mask   : %s\n", inet_ntoa(mask) );
	iprintf("dns1   : %s\n", inet_ntoa(dns1) );
	iprintf("dns2   : %s\n", inet_ntoa(dns2) );

	iprintf("ntp url: %s\n",ntpurl);

	struct hostent * ntphost = gethostbyname(ntpurl);
	if(ntphost == NULL) {
		iprintf("Error: failed to get hostname");
		goto idle;
	}

	iprintf("h_name : %s\n",ntphost->h_name);

	for(size_t i=0; ntphost->h_aliases[i] != NULL; i++) {
		iprintf("h_alias: %s\n",ntphost->h_aliases[i]);
	}

	/* Assert that we're dealing with IPv4 addresses, 32 bit lengths. */
	assert(ntphost->h_addrtype == AF_INET);
	assert(ntphost->h_length == 4);

	for(int i=0; ntphost->h_addr_list[i] != NULL; i++) {
		struct in_addr a = *(struct in_addr *)ntphost->h_addr_list[i];
		iprintf("h_addr : %s\n", inet_ntoa(a));
	}
	


	/* --- START OF coreSNTP EXAMPLE --- */
	/* This section is Copyright (c) Amazon Inc. or its affiliates. 
	 * Distribuited under the MIT license */

    /* @[code_example_sntp_init] */
    /* Memory for network buffer. */
    uint8_t networkBuffer[ SNTP_PACKET_BASE_SIZE ];
 
    /* Create UDP socket. */
    NetworkContext_t udpContext;
 
    udpContext.udpSocket = socket( AF_INET, SOCK_DGRAM, 0 );
	LogDebug(("UDP fd : %i",udpContext.udpSocket));
 
    /* Setup list of time servers. */
    SntpServerInfo_t pTimeServers[] =
    {
        {
            .port = SNTP_DEFAULT_SERVER_PORT,
            .pServerName = TEST_TIME_SERVER_1,
            .serverNameLen = strlen( TEST_TIME_SERVER_1 )
        },
        {
            .port = SNTP_DEFAULT_SERVER_PORT,
            .pServerName = TEST_TIME_SERVER_2,
            .serverNameLen = strlen( TEST_TIME_SERVER_2 )
        }
    };
 
    /* Set the UDP transport interface object. */
    UdpTransportInterface_t udpTransportIntf;
 
    udpTransportIntf.pUserContext = &udpContext;
    udpTransportIntf.sendTo = sntpUdpSend;
    udpTransportIntf.recvFrom = sntpUdpRecv;
 
    /* Context variable. */
    SntpContext_t context;
 
	LogDebug(("Initializing SNTP"));
	iprintf("Unix time is %llu\n", time(NULL));

    /* Initialize context. */
    SntpStatus_t status = Sntp_Init( &context,
                                     pTimeServers,
                                     sizeof( pTimeServers ) / sizeof( SntpServerInfo_t ),
                                     SERVER_RESPONSE_TIMEOUT_MS,
                                     networkBuffer,
                                     SNTP_PACKET_BASE_SIZE,
                                     sntpResolveDns,
                                     sntpGetTime,
                                     sntpSetTime,
                                     &udpTransportIntf,
                                     NULL );
 
    assert( status == SntpSuccess );
    /* @[code_example_sntp_init] */
 
    /* Calculate the polling interval period for the SNTP client. */
    /* @[code_example_sntp_calculatepollinterval] */
    uint32_t pollingIntervalPeriod;
 
    status = Sntp_CalculatePollInterval( SYSTEM_CLOCK_FREQUENCY_TOLERANCE_PPM,
                                         SYSTEM_CLOCK_DESIRED_ACCURACY_MS,
                                         &pollingIntervalPeriod );
    /* @[code_example_sntp_calculatepollinterval] */
    assert( status == SntpSuccess );

	while( 1 )
    {
        status = Sntp_SendTimeRequest( &context,
                                       rand() % UINT32_MAX,
                                       TIME_REQUEST_SEND_WAIT_TIME_MS );
		iprintf("Sntp_SendTimeRequest = %u\n", (unsigned int)status);
		if ( status != SntpSuccess ) continue;
        //assert( status == SntpSuccess );
 
        do
        {
            status = Sntp_ReceiveTimeResponse( &context, TIME_REQUEST_RECEIVE_WAIT_TIME_MS );
        } while( status == SntpNoResponseReceived );
 		iprintf("Sntp_ReceiveTimeResponse = %u\n", (unsigned int)status);
        if ( status != SntpSuccess ) continue;
		//assert( status == SntpSuccess );

		LogDebug(("SNTP Sucess!!!"));
		iprintf("RTC is %llu\n", time(NULL));

	    /* Delay of poll interval period before next time synchronization. */
		cont:
		break; 
		/* TODO: sleep() and usleep() seem to be broken? They return -1.
		 * Investigate.
		iprintf("pollingIntervalPeriod = %lu\n", pollingIntervalPeriod);
		uint32_t remaining = sleep( pollingIntervalPeriod );
		iprintf("remaining = %li\n", remaining);
		swiWaitForVBlank();
		 */
    }
 
	/* --- END OF coreSNTP EXAMPLE ---*/

	idle:
	while(1) {
		swiWaitForVBlank();
		int keys = keysDown();
		if(keys & KEY_START) break;
	}

	return 0;
}
