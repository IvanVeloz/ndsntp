/*
 * Copyright (C) 2024 Ivan Veloz.  All Rights Reserved.
 *
 * SPDX-License-Identifier: MIT
 * SPDX-FileContributor: Ivan Veloz, 2024
 */

#define CORE_SNTP_LOG_LEVEL 6
#define RTC_IS_GMT	false

#include <nds.h>
#include <unistd.h>
#include <dswifi9.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <time.h>
#include <core_sntp_client.h>
#include "core_sntp_callbacks.h"
#include "core_sntp_config.h"

/* Configuration constants for the SNTP client. */
#define NTP_TIMEOUT						3000
#define NTP_SEND_WAIT_TIME_MS 			2000
#define NTP_RECEIVE_WAIT_TIME_MS		1000

/* Function macros */
#define IF_DIAGNOSTICS					\
	if(keysCurrent() & (KEY_L|KEY_R))

/* Environment variables *
 * These are used by some POSIX functions and are normally provided by the
 * operating system. They are not provided by the NDS homebrew launchers though.
 * So we provide the TZ variable ourselves for `localtime()` to do all timezone
 * conversions.
 */
static char ndsntp_env_tz[256] = "";
static char *ndsntp_env[] = {
	ndsntp_env_tz,
	NULL
};
extern char **environ = &ndsntp_env[0];	// Provide our own environment variables
extern char *tzname[2];

/* Global types */
struct Tz {
	int8_t hour;
	uint8_t minute;
};

enum Menu { MENU_TZ, MENU_SYNCING, MENU_SYNCED, MENU_EXIT };

/* Function prototypes */
void spinloop(void);
unsigned int sleeprtc(unsigned int seconds);
void printIpInfo(void);
int printNsLookup(void);
void printEnviron(void);
int syncTime(int retries);
enum Menu displayTZMenu(void);
enum Menu displaySyncedMenu(void);

/* Global vairiables */
const char * ntpurl = "us.pool.ntp.org";

int main(void) {

	consoleDemoInit();

	printf("Connecting to WLAN\n");
	if(!Wifi_InitDefault(WFC_CONNECT)) {
		printf("WFC connection failed. Check your wireless settings.\n");
		spinloop();
		goto end;
	}

	scanKeys();
	IF_DIAGNOSTICS {
		printIpInfo();
		sleeprtc(2);
	}

	scanKeys();
	IF_DIAGNOSTICS {
		if(printNsLookup()) spinloop();
		else sleeprtc(2);
	}

	scanKeys();
	IF_DIAGNOSTICS {
		printEnviron();
		tzset();
		printf("%s\n%s\n",tzname[0],tzname[1]);
		spinloop();
		printf("\x1b[2J"); // Clear console
	}

	enum Menu menu = MENU_TZ;
	while( 1 )
    {
		switch(menu) {
			case MENU_TZ:
				menu = displayTZMenu();
				break;
			case MENU_SYNCING:
				swiWaitForVBlank();
				scanKeys();
				printf("\x1b[2J"); // Clear console
				printf("\n\n");
				if(syncTime(5)) {
					printf("Couldn't connect to time server(s)!\n");
				}
				IF_DIAGNOSTICS {
					sleeprtc(2);
				}
				menu = MENU_SYNCED;
				break;
			case MENU_SYNCED:
				menu = displaySyncedMenu();
				break;
			case MENU_EXIT:
				goto end;
			default:
				menu = MENU_TZ;
		}



		printf("\n\n");
    }
	end:
	return 0;
}

/* Do nothing until a key is pressed.
 */
void spinloop(void) {
	while(1) {
		swiWaitForVBlank();
		scanKeys();
		int keys = keysDown();
		if(keys) break;	
	}
}

/* Delay for a number of seconds, using the RTC as time source.
 * This way we avoid setting up timers. The actual time slept may be
 * a fraction of a second more than requested (e.g. if 1 second is asked 
 * right after the clock ticks, the function will sleep 1 second plus the
 * fraction elapsed right after the clock ticked). Returns 0.
 */
unsigned int sleeprtc(unsigned int seconds) {
	for(time_t t = time(NULL), l=t+seconds+1; t<l; t = time(NULL)) {
		swiWaitForVBlank();	// I assume this is more energy efficient
	}
	return 0;
}

/* Print IP address information for diagnostics.
 */
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

/* Lookup the NTP hostname and print the entry(ies) given by the DNS server.
 */
int printNsLookup(void) {
	struct hostent * ntphost = gethostbyname(ntpurl);
	if(ntphost == NULL) {
		printf("Error: failed to get hostname");
		return -1;
	}

	/* Assert that we're dealing with IPv4 addresses, 32 bit lengths. */
	assert(ntphost->h_addrtype == AF_INET);
	assert(ntphost->h_length == 4);
	printf("h_name : %s\n",ntphost->h_name);
	for(size_t i=0; ntphost->h_aliases[i] != NULL; i++) {
		printf("h_alias: %s\n",ntphost->h_aliases[i]);
	}
	for(int i=0; ntphost->h_addr_list[i] != NULL; i++) {
		struct in_addr a = *(struct in_addr *)ntphost->h_addr_list[i];
		printf("h_addr : %s\n", inet_ntoa(a));
	}
	return 0;
}

/* Print all environment variables. This works on Linux and Unix too! Just copy
 * and paste. Very cool.
 */
void printEnviron(void)
{
	for(size_t i=0; environ[i]!=NULL; i++)
		printf("%s\n",environ[i]);
}


/* Connect to an NTP server and set the time (assumes locale is set; if not set
 * it defaults to UTC).
 */
int syncTime(int retries)
{
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
	if(status != SntpSuccess) {
		LogError(("Failed to initialize SNTP.\n"));
		return -1;
	}

	int i,j;
	for(i=0, j=0; i<retries; i++) {
		status = Sntp_SendTimeRequest( &sntpContext,
										rand() % UINT32_MAX,
										NTP_SEND_WAIT_TIME_MS );
		if ( status != SntpSuccess ) continue;
		do {
            status = Sntp_ReceiveTimeResponse( &sntpContext, NTP_RECEIVE_WAIT_TIME_MS );
        } while( status == SntpNoResponseReceived );
		if ( status != SntpSuccess ) continue;
		break;
	}

	if(i>=retries || j>=retries) {
		LogError(("Failed to request SNTP time.\n"));
		return -1;
	}
	else 
		return 0;
}

/* Display the timezone setting dialog. 
 * @returns an `enum Menu` with the next menu that should be displayed.
 */
enum Menu displayTZMenu(void)
{
	enum Selection {s_hour, s_minute};

	static enum Selection sel = s_hour;
	static struct Tz tz = {.hour=0, .minute=0};

	swiWaitForVBlank();
	printf("\x1b[2J"); // Clear console
	const int coord_x[2] = {
		5, 8
	};
	printf("\n\nTimezone:\n\n");
	printf("\x1b[%dC^\n", coord_x[sel]);
	printf("UTC%+03i:%02u\n", tz.hour, tz.minute%60);
    printf("\x1b[%dCv\n", coord_x[sel]);

	printf(	"\n\n\n\n\n\n\n\n\n\n\n\n"
			"Press A to sync time.\n"
			"Press Start to exit.");

	scanKeys();
	uint16_t keys = keysDownRepeat();

	if(keys & KEY_A) {
		tz.minute = tz.minute % 60;
		
		snprintf(	ndsntp_env_tz,sizeof(ndsntp_env_tz),
					"TZ=<UTC%+03i%02u>%+03i:%02u",
					tz.hour, tz.minute, -tz.hour, tz.minute);
					// POSIX uses the opposite sign compared to ISO
		tzset();	// Change the system's timezone
		IF_DIAGNOSTICS {
			printf("\n");
			printEnviron();
			spinloop();
		}
		IF_DIAGNOSTICS {
			printf("%s\n%s\n",tzname[0],tzname[1]);
			spinloop();
		}
		
		return MENU_SYNCING;
	}
	else if(keys & KEY_LEFT) {
		if(sel>s_hour) sel--;
	}
	else if(keys & KEY_RIGHT) {
		if(sel<s_minute) sel++;
	}
	else if(keys & KEY_UP) {
		switch(sel) {
			case s_hour:
				if(tz.hour >= 16) tz.hour = 16;
				else tz.hour += 1;
				break;
			case s_minute:
				if(tz.minute >= 120) tz.minute = 120;
				else if(tz.minute >= 60) tz.minute += 1;
				else tz.minute += 15;
				break;
		}
	}
	else if(keys & KEY_DOWN) {
		switch(sel) {
			case s_hour:
				if(tz.hour <= -16) tz.hour = 16;
				else tz.hour -= 1;
				break;
			case s_minute:
				if(tz.minute <= 0) tz.minute = 0;
				else if(tz.minute > 60) tz.minute -= 1;
				else tz.minute -= 15;
				break;
		}
	}
	else if(keys & KEY_START) {
		return MENU_EXIT;
	}

	return MENU_TZ;

}

enum Menu displaySyncedMenu(void)
{
	char str[100];
	time_t t = time(NULL);
	struct tm *tmp = RTC_IS_GMT? localtime(&t) : gmtime(&t);
	swiWaitForVBlank();
	printf("\x1b[2J"); // Clear console
	printf("\n\nCurrent time:\n\n\n");
	if (strftime(str, sizeof(str), "%Y-%m-%dT%H:%M:%S%z", tmp) == 0)
		snprintf(str, sizeof(str), "Failed to get time");
	printf("%s\n", str);
	printf("\n\n\n\n\n\n\n\n\n\n\n\n");
	printf("Press A to sync again.\n"
		"Press B to go back.\n"
		"Press Start to exit.\n");
	scanKeys();
	int keys = keysDown();
	if(keys & KEY_START) return MENU_EXIT;
	if(keys & KEY_A) return MENU_SYNCING;
	if(keys & KEY_B) return MENU_TZ;
	return MENU_SYNCED;
}
