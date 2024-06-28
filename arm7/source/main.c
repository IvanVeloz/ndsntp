// SPDX-License-Identifier: CC0-1.0
//
// SPDX-FileContributor: Antonio Niño Díaz, 2023

#include <stdio.h>
#include <time.h>

#include <dswifi7.h>
#include <nds.h>
#include <maxmod7.h>

volatile bool exit_loop = false;

void power_button_callback(void)
{
    exit_loop = true;
}

void vblank_handler(void)
{
    inputGetAndSend();
    Wifi_Update();
}

void fifo_handler_datamsg_time_date(int num_bytes, void *userdata)
{
    rtcTimeAndDate rtc_time_date;
    fifoGetDatamsg(FIFO_USER_01, sizeof(rtc_time_date), (void *)&rtc_time_date);

    uint32_t ok = 0;

    if (rtcTimeAndDateSet(&rtc_time_date) == 0)
        ok = 1;

    // Read the RTC to get the new date instead of assuming the write succeeded
    resyncClock();

    fifoSendValue32(FIFO_USER_01, ok);
}

void fifo_handler_datamsg_time(int num_bytes, void *userdata)
{
    rtcTime rtc_time;
    fifoGetDatamsg(FIFO_USER_02, sizeof(rtc_time), (void *)&rtc_time);

    uint32_t ok = 0;

    if (rtcTimeSet(&rtc_time) == 0)
        ok = 1;

    // Read the RTC to get the new date instead of assuming the write succeeded
    resyncClock();

    fifoSendValue32(FIFO_USER_01, ok);
}

int main(int argc, char *argv[])
{
    // Initialize sound hardware
    enableSound();

    // Read user information from the firmware (name, birthday, etc)
    readUserSettings();

    // Stop LED blinking
    ledBlink(0);

    // Using the calibration values read from the firmware with
    // readUserSettings(), calculate some internal values to convert raw
    // coordinates into screen coordinates.
    touchInit();

    irqInit();
    irqSet(IRQ_VBLANK, vblank_handler);

    fifoInit();

    installWifiFIFO();
    installSoundFIFO();
    if (isDSiMode())
        installCameraFIFO();
    installSystemFIFO(); // Sleep mode, storage, firmware...

    // Initialize Maxmod. It uses timer 0 internally.
    mmInstall(FIFO_MAXMOD);

    // This sets a callback that is called when the power button in a DSi
    // console is pressed. It has no effect in a DS.
    setPowerButtonCB(power_button_callback);

    // Read current date from the RTC and setup an interrupt to update the time
    // regularly. The interrupt simply adds one second every time, it doesn't
    // read the date. Reading the RTC is very slow, so it's a bad idea to do it
    // frequently.
    initClockIRQTimer(3);

    irqEnable(IRQ_VBLANK);

    // This channel will listen to messages from the ARM9 with a time and date
    fifoSetDatamsgHandler(FIFO_USER_01, fifo_handler_datamsg_time_date, NULL);
    // This one will only change the time
    fifoSetDatamsgHandler(FIFO_USER_02, fifo_handler_datamsg_time, NULL);

    while (!exit_loop)
    {
        printf("Hello from ARM7\n");
        const uint16_t key_mask = KEY_START;
        uint16_t keys_pressed = ~REG_KEYINPUT;

        if ((keys_pressed & key_mask) == key_mask)
            exit_loop = true;

        swiWaitForVBlank();
    }

    return 0;
}
