# Working with Code
One of the major strengths of the ESP32 is its ability to run WiFi technology.  This then
poses the question, what should a Duktape environment "do" with WiFi at startup?

Let us think about how we can load and run programs in a Duktape environment.  Currently
we can:

* Telnet into the device (which requires WiFi)
* Serial connect into the device

But are there other options?  What about the idea that we can run a built in Web Server
that serves up web pages that are themselves an editor for JavaScript?