# ESP32-Duktape - Documentation
This folder contains the documentation and project notes for the ESP32-Duktape project.

## Status
The project was started in November 2016 and hence is still extremely new.  Experience and feedback with it and upon
it is very light.  Any comments should be emailed to kolban1@kolban.com.

## Binary install
Instructions for downloading and installing a binary version of ESP23-Duktape can be found [here](installation.md).

## Running ESP32-Duktape
After flashing your ESP32 with the images, reboot the device.  At this point the device will start and
begin running ESP32-Duktape.  Since this is the first time it has started, it will not know how to connect
to your WiFi environment.  Because network connection is required to attach a terminal to it, there is some
setup required.  Because ESP32-Duktape has determined it doesn't know how to connect to your WiFi, it becomes
an access point ready for configuration.  Take your smart phone and look for available WiFi networks.  You
will find one called "ESP32 Duktape".  Connect your phone to that network.  Now you should launch a browser
on your phone and visit "http://192.168.4.1" (Note: Your own access point number may differ).  You will now be presented with a page
into which you can enter your local WiFi network name (SSID) and connection password.  Once submitted,
reboot your ESP32 again and this time it will connect to the network using the information you supplied.
Now your ESP32 is addressable.  You can test this with a ping command:

```
ping <IP Address of your ESP32>
```

which should respond successfully.  At this point, your ESP32 is up and running and ready to execute JavaScript.
As a quick test, open a "telnet" session to the IP address of your ESP32 and you should see an `ESP32-Duktape>`
prompt.  Enter JavaScript and you are off to the races.

## Troubleshooting
Here is where we will record troubleshooting tips by category.  In general, attach a serial terminal to the serial
connections of your ESP32 as that is where diagnostics are written.

### Network connection
* If your ESP32 is configured to connect to a specific access point and it is no longer available, the device will go
back to being an access point for reconfiguration.
* You can force your ESP32 to become an access point for reconfiguration by holding GPIO25 high (3.3V) at boot.