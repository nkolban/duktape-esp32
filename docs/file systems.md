# File systems
ESP32-Duktape is heavily dependent on file systems being available on the ESP32.  Not only
is it "nice" to have a file system that your own JavaScript applications can read and write,
the project is mostly written in JavaScript itself.  The framework that provides networking
and timers and much more provides "modules" (written in JavaScript) that are loaded and
compiled into the solution only when and as needed.  If you don't need GPIO in your application,
then it isn't built in and saves our precious RAM.  In order for modules to work well, we need
to have a file system upon which the source (JavaScript) of those modules can be loaded from.

Initially, the plan was to use "SPIFFS" exclusively as the base file system.  Although not
necessarily the fastest, that wasn't the primary concern.  However an interesting technical
facet comes into play.  In order for the JavaScript Duktape engine to be able to compile a module
it must load the source of that module.  That makes sense.  However, since SPIFFS does not keep
the file data in a contiguous area of space on the flash memory, we have to first use POSIX
APIs to load the module source into RAM and then compile the module from that RAM copy.  As
such we go from:

```
Flash (source code) -> RAM (source code) -> RAM (compiled byte code)
```

The problem with this is that in order to end up with the compiled code, we need to have a
full copy of the source code in RAM.  If we are tight on RAM, then having the source code
be resident in RAM at the same time as the compiled byte code may be too much.

A different file system type called "ESPFS" is available to us.  The benefit of this file system
is that a "file" contained on it can be accessed through memory mapping.  This means that the
file can be read at runtime by the Duktape internal compiler without first loading
the source into RAM.  The story then becomes:

```
Flash (source code) -> RAM (compiled byte code)
```

That is a dramatic saving in RAM.  Unfortunately, we can't just use ESPFS as our file
system in place of SPIFFS because ESPFS is read-only.  We can built our file system image
of files at Duktape build time but can't dynamically write to it.

The solution (if we can call it that) is that at present we employ *both* types of file system.
The files that we wish to bundle with an ESP32-Duktape installation are converted into
an ESPFS image *and* a SPIFFS image and *both* are loaded into flash.  Since our ESP32 boards
have enough flash (4MB is typical) we aren't in a squeeze there.

This design is ripe for improvement and there are thoughts by the Duktape designer for a future
enhancement which might allow compiled byte code to itself reside in flash.  If/when that day
comes, we will look again at how file systems are managed. 

The current SPIFFS implementation comes from:

[https://github.com/whitecatboard/Lua-RTOS-ESP32/tree/master/components](https://github.com/whitecatboard/Lua-RTOS-ESP32/tree/master/components)

The current ESPFS implementation comes from:

[https://github.com/Spritetm/libesphttpd/tree/master/espfs](https://github.com/Spritetm/libesphttpd/tree/master/espfs)