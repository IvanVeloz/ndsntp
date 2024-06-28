# ndsntp
NTP client for the Nintendo DS.

Uses the coreNTP library made by Amazon for the FreeRTOS project. The library has been ported and targets one second precision (as that is the resolution for the NDS's real time clock).

The project targets BlocksDS and real hardware. You can build it by installing the BlocksDS SDK and typing `make`. Releases will be available in the near future.

As of this version, the project can get an UNIX timestamp with UTC time from the server. Next steps are (in no particular order):
* Reading a configuration file with the desired UTC offset
    - Reading a configuration file with the desired TZ timezone, and having the offset calculated from that.
* Implementing a mitigation for time-bombed R4 clones. Some R4 clone flashcarts stop working after the year 2024. What some people do is set the clock behind the real time, for example, setting the clock to the year 2014 instead of 2024. We can support this workaround by storing an offset instead of manipulating the real time clock.
