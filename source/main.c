/*
 * Copyright (C) 2024 Ivan Veloz.  All Rights Reserved.
 *
 * SPDX-License-Identifier: MIT
*/

#include <nds.h>
#include <dswifi9.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <assert.h>
#include <core_sntp_client.h>

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

	idle:
	while(1) {
		swiWaitForVBlank();
		int keys = keysDown();
		if(keys & KEY_START) break;
	}

	return 0;
}
