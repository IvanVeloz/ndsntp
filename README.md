# ndsntp
NTP client for the Nintendo DS. Download [here](https://github.com/IvanVeloz/ndsntp/releases).

## Usage
* Start the app
* Use the arrow keys to configure your local timezone.
  - The minutes go up in steps of 15, but if you keep pressing up, they go back to zero and increment in steps of 1.
* Press A to set the time.
* Press start to go exit the app, A to sync again, or B to go back to the start.

Tip: you can get diagnostics information by holding down the L or R button before starting the app. Some information dismisses itself after 2 seconds, other information stays until you press a button.

### Background information
Uses the coreNTP library made by Amazon for the FreeRTOS project. The library has been ported and targets one second precision (as that is the resolution for the NDS's real time clock). The project targets BlocksDS and real hardware. You can build it by installing the BlocksDS SDK and typing `make`.

### Project status
As of this version, the project can get the time from an NTP server, apply your timezone settings, and store it in the NDS real time clock. You provide your timezone (for example UTC-04) with an user interface.

Next steps are (in no particular order):
* Reading a configuration file with the desired UTC offset
* Using timezones from the Tz database (e.g. America/New_York or Asia/Shanghai) instead of UTC offsets (e.g. UTC+04:00 or UTC+8:00).
* Implementing a mitigation for time-bombed R4 clones. Some R4 clone flashcarts stop working after the year 2024. What some people do is set the clock behind the real time, for example, setting the clock to the year 2014 instead of 2024. We can support this workaround by storing an offset instead of manipulating the real time clock.
