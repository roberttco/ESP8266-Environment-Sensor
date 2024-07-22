This code base implements an ESP8266 based "environment sensor".  The code is
intended to minimize average energy consumption by managing the WiFi radio operation
and using deep sleep.

Modify the config.h file before compiling and installing on hardware.  this code
is tested on a WeMos D1 Mini and custom hardware.

Additional improvements include configuring a static IP address to further reduce
the amount of time the wifi radio is enabled.