#ESP32-Duktape installation
This guide assumes that you have obtained (or built) a binary ready to flash
into your ESP32.  The end goal will be to have ESP32 running JavaScript and
a built in development environment.

The distribution consists of the following files:

* `esp32-duktape.bin` - The primary application binary.
* `partitions_singleapp.bin` - The partitions description for the application.
* `bootloader.bin` - The bootloader for the application.
* `spiffs.img` - An image of JavaScript files in SPIFFS format.
* `espfs.img` - An image of JavaScript files in ESPFS format.

The load addresses are:

|file                     |Address
+-------------------------+--------
|bootloader.bin           |0x1000
|partitions_singleapp.bin |0x8000
|esp32-duktape.bin        |0x10000
|spiffs.img               |0x180000
|espfs.img                |0x360000

For example, on Linux with the ESP32 in flash mode, we can run the following:
```
$ python <ESP-IDF>/components/esptool_py/esptool/esptool.py \
 --chip esp32 --port "/dev/ttyUSB0" --baud 115200 write_flash --flash_mode "dio" \
 --flash_freq "40m" --flash_size "4MB" --compress \
 0x1000   bootloader.bin \
 0x8000   partitions_singleapp.bin \
 0x10000  esp32-duktape.bin \
 0x180000 spiffs.img \
 0x360000 espfs.img
```

When all is complete, the ESP32 can be rebooted and we are ready to go.

#First boot
In order to use the ESP32-Duktape program, we need to be WiFi connected (at least
initially).  However, in order for an application to connect to *your* WiFi access point
we need to know its name (SSID) and the password you use to connect to it.  That means
that you have to supply it.  Ahh ... but wait, how do you supply that information when
you don't have network access?

When ESP32-Duktape boots, it looks in the Non Volatile Storage area for the network name
and password it should use.  Since on first boot, that is not present, the ESP32 itself
becomes a WiFi access point.  Here you can connect your phone (or other mobile device)
to it and bring up a browser point to <Insert IP address here>.

From there, you will be presented with the opportunity to enter a network name and
password.  Now, when you reboot your ESP32, it will connect to that network.  If the 
network connection fails (because you are now somewhere else or you change your password),
then the ESP32 will again reset to being an access point.

It is strongly recommend that you keep the console log up while the ESP32 is running
so that you can see the messages being written.

#Speeding up ESP32-Duktape
You may find that the ESP32-Duktape program feels a little sluggish.  The reason for this
is that at this early stage, a *lot* of debugging is still enabled and this is logged to
the ESP32 console ... which means that we are logging a lot of text at 115200 baud through
a serial port.  That slows us down.  We can eliminate the debug logging by running the following
JavaScript:

```
ESP32.setLogLevel("*", "error");
```

You will find that performance jumps dramatically but now you won't get any console output.  To see
the console output again, run:

```
ESP32.setLogLevel("*", "debug");
```