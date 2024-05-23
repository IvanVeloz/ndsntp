# ndsntp
NTP client for the Nintendo DS.

Uses the coreNTP library made by Amazon for the FreeRTOS project. The library has been ported and targets one second precision (as that is the resolution for the NDS's real time clock).

Currently, the main branch contains code for the DevKitPro SDK for the Nintendo DS. A branch supporting the BlocksDS SDK is available, called `blocksds`. Future development will focus on BlocksDS because it's in active development and I want to support this.
