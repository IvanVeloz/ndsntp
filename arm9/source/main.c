/*
 * Copyright (C) 2024 Ivan Veloz.  All Rights Reserved.
 *
 * SPDX-License-Identifier: MIT
 * SPDX-FileContributor: Ivan Veloz, 2024
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
#include <time.h>
#include <core_sntp_client.h>
#include "core_sntp_callbacks.h"
#include "core_sntp_config.h"

/* Configuration constants for the SNTP client. */

#define NTP_TIMEOUT						3000
#define NTP_SEND_WAIT_TIME_MS 			2000
#define NTP_RECEIVE_WAIT_TIME_MS		1000

#define IF_DIAGNOSTICS					\
	swiWaitForVBlank();					\
	scanKeys();							\
	if(keysDown() & (KEY_L|KEY_R))

const char * ntpurl = "us.pool.ntp.org";

void spinloop(void);
void printIpInfo(void);
unsigned int sleeprtc(unsigned int seconds);

int main(void) {

	consoleDemoInit();

	if(!Wifi_InitDefault(WFC_CONNECT)) {
		printf("WFC connection failed. Check your wireless settings.\n");
		spinloop();
		goto end;
	}
	
	IF_DIAGNOSTICS {
		printIpInfo();
	}

	struct hostent * ntphost = gethostbyname(ntpurl);
	if(ntphost == NULL) {
		printf("Error: failed to get hostname");
		spinloop();
		goto end;
	}

	/* Assert that we're dealing with IPv4 addresses, 32 bit lengths. */
	assert(ntphost->h_addrtype == AF_INET);
	assert(ntphost->h_length == 4);

	IF_DIAGNOSTICS {
		printf("h_name : %s\n",ntphost->h_name);
		for(size_t i=0; ntphost->h_aliases[i] != NULL; i++) {
			printf("h_alias: %s\n",ntphost->h_aliases[i]);
		}
		for(int i=0; ntphost->h_addr_list[i] != NULL; i++) {
			struct in_addr a = *(struct in_addr *)ntphost->h_addr_list[i];
			printf("h_addr : %s\n", inet_ntoa(a));
		}
	}
	
	uint8_t netBuffer[SNTP_PACKET_BASE_SIZE];
	NetworkContext_t netContext = {
		.udpSocket = socket(AF_INET, SOCK_DGRAM, 0)
	};
	SntpServerInfo_t server = {
		.port = SNTP_DEFAULT_SERVER_PORT,
		.pServerName = ntpurl,
		.serverNameLen = strlen(ntpurl)
	};
    UdpTransportInterface_t udpTransportIntf = {
		.pUserContext = &netContext,
		.sendTo = sntpUdpSend,
		.recvFrom = sntpUdpRecv
	};
	SntpContext_t sntpContext;
	
    SntpStatus_t status = Sntp_Init( &sntpContext,
                                     &server,
                                     sizeof(server) / sizeof(SntpServerInfo_t),
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

void spinloop(void) {
	while(1) {
		swiWaitForVBlank();
		int keys = keysDown();
		if(keys) break;	
	}
}

void printIpInfo(void) {
	struct in_addr ip, gateway, mask, dns1, dns2;
	ip = Wifi_GetIPInfo(&gateway, &mask, &dns1, &dns2);
	printf("ip     : %s\n", inet_ntoa(ip) );
	printf("gateway: %s\n", inet_ntoa(gateway) );
	printf("mask   : %s\n", inet_ntoa(mask) );
	printf("dns1   : %s\n", inet_ntoa(dns1) );
	printf("dns2   : %s\n", inet_ntoa(dns2) );
	printf("ntp url: %s\n",ntpurl);
}

/* Delay for a number of seconds, using the RTC as time source.
 * This way we avoid setting up timers. The actual time slept may be
 * a fraction of a second more than requested (e.g. if 1 second is asked 
 * right after the clock ticks, the function will sleep 1 second plus the
 * fraction elapsed right after the clock ticked). Returns 0.
 */
unsigned int sleeprtc(unsigned int seconds) {
	for(time_t t = time(NULL), l=t+seconds+1; t<l; t = time(NULL));
	return 0;
}

