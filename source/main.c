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

/* Configuration constants for the SNTP client. */

#define NTP_TIMEOUT						3000
#define NTP_SEND_WAIT_TIME_MS 			2000
#define NTP_RECEIVE_WAIT_TIME_MS		1000

const char * ntpurl = "us.pool.ntp.org";

void spinloop() {
	while(1) {
		swiWaitForVBlank();
		int keys = keysDown();
		if(keys) break;	
	}
}

int main(void) {

	struct in_addr ip, gateway, mask, dns1, dns2;

	consoleDemoInit();

	printf("Connecting via WFC data\n");

	if(!Wifi_InitDefault(WFC_CONNECT)) {
		printf("Connection failed.\n");
		spinloop();
		goto end;
	}

	printf("Connected.\n");

	ip = Wifi_GetIPInfo(&gateway, &mask, &dns1, &dns2);
	
	printf("ip     : %s\n", inet_ntoa(ip) );
	printf("gateway: %s\n", inet_ntoa(gateway) );
	printf("mask   : %s\n", inet_ntoa(mask) );
	printf("dns1   : %s\n", inet_ntoa(dns1) );
	printf("dns2   : %s\n", inet_ntoa(dns2) );

	printf("ntp url: %s\n",ntpurl);

	struct hostent * ntphost = gethostbyname(ntpurl);
	if(ntphost == NULL) {
		printf("Error: failed to get hostname");
		spinloop();
		goto end;
	}

	printf("h_name : %s\n",ntphost->h_name);

	for(size_t i=0; ntphost->h_aliases[i] != NULL; i++) {
		printf("h_alias: %s\n",ntphost->h_aliases[i]);
	}

	/* Assert that we're dealing with IPv4 addresses, 32 bit lengths. */
	assert(ntphost->h_addrtype == AF_INET);
	assert(ntphost->h_length == 4);

	for(int i=0; ntphost->h_addr_list[i] != NULL; i++) {
		struct in_addr a = *(struct in_addr *)ntphost->h_addr_list[i];
		printf("h_addr : %s\n", inet_ntoa(a));
	}
	
	uint8_t netBuffer[SNTP_PACKET_BASE_SIZE];
	NetworkContext_t netContext;
	netContext.udpSocket = socket(AF_INET, SOCK_DGRAM, 0);

	LogDebug(("UDP fd : %i",con.udpSocket));

	SntpServerInfo_t server = {
		.port = SNTP_DEFAULT_SERVER_PORT,
		.pServerName = ntpurl,
		.serverNameLen = strlen(ntpurl)
	};

    UdpTransportInterface_t udpTransportIntf;
 
    udpTransportIntf.pUserContext = &netContext;
    udpTransportIntf.sendTo = sntpUdpSend;
    udpTransportIntf.recvFrom = sntpUdpRecv;

	SntpContext_t sntpContext;
	
    SntpStatus_t status = Sntp_Init( &sntpContext,
                                     &server,
                                     sizeof( server ) / sizeof( SntpServerInfo_t ),
                                     NTP_TIMEOUT,
                                     netBuffer,
                                     SNTP_PACKET_BASE_SIZE,
                                     sntpResolveDns,
                                     sntpGetTime,
                                     sntpSetTime,
                                     &udpTransportIntf,
                                     NULL );
	
	assert(status == SntpSuccess);

	while( 1 )
    {
        status = Sntp_SendTimeRequest( &sntpContext,
                                       rand() % UINT32_MAX,
                                       NTP_SEND_WAIT_TIME_MS );
		printf("Sntp_SendTimeRequest = %u\n", (unsigned int)status);
		if ( status != SntpSuccess ) continue;
 
        do
        {
            status = Sntp_ReceiveTimeResponse( &sntpContext, NTP_RECEIVE_WAIT_TIME_MS );
        } while( status == SntpNoResponseReceived );
 		printf("Sntp_ReceiveTimeResponse = %u\n", (unsigned int)status);
        if ( status != SntpSuccess ) continue;

		printf("\n\n\nRTC is %llu\n", time(NULL));
		printf("Press A to sync again.\n"
			   "Press Start to power off.\n");
		while(1) {
			swiWaitForVBlank();
			scanKeys();
			int keys = keysDown();
			if(keys & KEY_START) goto end;
			if(keys & KEY_A) break;
		}
		printf("\n\n");
    }
	end:
	return 0;
}
