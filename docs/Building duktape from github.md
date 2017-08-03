# Building duktape from Github
To use the latest versions of Duktape, we will want to build our own version from a release of the
source contained on Github.  Here is the recipe.

1. Visit the release/build site for Duktape found here

[https://github.com/svaarala/duktape/releases]

2. Choose a current release and download the compressed file (duktape-x.x.x.tar.xz)

3. Extract the content.  You will now have a directory called `duktape-x.x.x`.  Move that directory into the  `components` folder of the
ESP32-Duktape project renaming it to be `duktape`.



We now have the Duktape source where it needs to be (`$ESP32_DUKTAPE/components/duktape`) however it needs some
fixup.

4. Create the `component.mk` file.  In order to be part of an ESP-IDF build, there must be a `component.mk`
in the `duktape` directory.  This should contains:

```
COMPONENT_ADD_INCLUDEDIRS:=src extras/module-duktape examples/debug-trans-socket
COMPONENT_SRCDIRS:=src extras/module-duktape examples/debug-trans-socket
```

5. Change the `duk_trans_socket_unix.c` file.  This is a sample file that provides Debug support for Duktape
using TCP/IP sockets.  Unfortunately, ESP32 isn't exactly compliant (yet).  Specifically, the header
file called `<netinet/in.h>` is not present in ESP32.   In addition, we need to add

```
#define USE_SELECT
```

at the top of the file as ESP32 doesn't support the `poll()` system call.

6. Remove the file `$ESP32_DUKTAPE/components/duktape/examples/debug-trans-socket/duk_trans_socket_windows.c`.  The
ESP-IDF build systems wants to compile ALL files in a directory ... and this means that files we don't
want (such as Windows support) will also be compiled by default.


### Optional
The following step may be of use if you are attempting to run on the WROOM.  I strongly recommend not doing that
but instead run on the WROVER ESP32 module.

7. Now we configure our version of duktape to use the low memory profile:

```
python tools/configure.py \
    --config-metadata config/ \
    --source-directory src-input \
    --option-file components/duktape/config/examples/low_memory.yaml \
    --option-file data/duktape/ESP32-Duktape.yaml \
    --output-directory src
```

ESP32-Duktape uses two distinct file systems.  One called "SPIFFS" and the other called "ESPFS".  The source
and tools for these are:

* [spiffs](https://github.com/whitecatboard/Lua-RTOS-ESP32/tree/master/components)
* [espfs](https://github.com/Spritetm/libesphttpd/tree/master/espfs)

However the tools needed to build them are supplied in the `./bin` directory.