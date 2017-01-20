#ESP32-Duktape - Documentation
This folder contains the documentation and project notes for the ESP32-Duktape project.

##Status
The project was started in November 2016 and hence is still extremely new.  Experience and feedback with it and upon
it is very light.  Any comments should be emailed to kolban1@kolban.com.

##Binary install
A flashable binary is available for the ESP32.  We assume you have experience in using the ESP32 environment.  To
flash ESP32-Duktape, run the following command:

```
esptool.py --chip esp32 --port "/dev/ttyUSB0" --baud 115200  write_flash -z \
  --flash_mode "dio" --flash_freq "40m"  --flash_size "2MB" \
  0x1000 bootloader.bin 0x10000 app-template.bin 0x8000 partitions_singleapp.bin
```

The binary is supplied as a tar/gzip of three distinct files:

* `bootloader.bin`
* `esp32-duktape.bin`
* `partitions-singleapp.bin`
* `spiffs.img`
* `espfs.img`

The file can be download at:

[http://www.neilkolban.com/esp32/downloads/esp32-duktape-2017-01-20.tar.gz](http://www.neilkolban.com/esp32/downloads/esp32-duktape-2017-01-20.tar.gz)


##Running ESP32-Duktape
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