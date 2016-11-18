# esp32-mongoose
This is a test of creating Mongoose as an ESP32 component.  To compile
ESP32 application code to include mongoose, we must add the following
compile time flags:

```
CFLAGS += -DCS_PLATFORM=3 -DMG_DISABLE_DIRECTORY_LISTING=1 -DMG_DISABLE_DAV=1 -DMG_DISABLE_CGI=1
```

A Mongoose sample for ESP32 can be found here:

[https://github.com/nkolban/esp32-snippets/tree/master/mongoose/webserver](https://github.com/nkolban/esp32-snippets/tree/master/mongoose/webserver)