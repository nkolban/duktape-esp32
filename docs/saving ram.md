# Saving RAM
While the ESP32 has a whopping 512K RAM, apparently a maximum on about 380K can be used as data ram (heap) and experience
is showing that we start with about 150K.  Take away that needed by basic Duktape and other goodies and we are left
with only about 70K for our apps ... and this dwindles fast.  As such, we need to look at ways in which we
can reduce RAM utilization.  Note that it appears that we are not short on Flash, so ROM storage is not a concern.

An excellent paper on Duktape and RAM utilization can be found here

[https://github.com/svaarala/duktape/blob/master/doc/low-memory.rst](https://github.com/svaarala/duktape/blob/master/doc/low-memory.rst)


## Optional modules
In order to save RAM, we can choose to include or exclude Duktape components.  If we had unlimited RAM, we would likely
enable all components and all performance optimizations ... but they come at the cost of RAM so we must trade-off reduced
function or less optimal run-time implementations in favor of reduced RAM.  Some of the options we can switch on or off
include:

* `DUK_USE_VERBOSE_ERRORS` - Quality of error reporting at the JS level.
* `DUK_USE_TRACEBACKS`
* `DUK_USE_AUGMENT_ERROR_CREATE`
* `DUK_USE_FUNC_FILENAME_PROPERTY`
* `DUK_USE_PC2LINE`

To enable or disable these settings, edit the file called `./data/duktape/ESP32-Duktape.yaml` and change settings from
`true` to `false` or from `false` to `true`.  Once done, we have to tell Duktape to use the new settings by running
`make duktape_configure` and then rebuilding ESP32-Duktape.


## Running Duktape from flash
One of the possibilities for RAM improvements is the ability to run compiled Duktape applications
from flash memory.

Steps:

* Enable some options including
 * DUK_USE_ROM_STRINGS
 * DUK_USE_ROM_OBJECTS
 * DUK_USE_GLOBAL_INHERIT

When building Duktape, we need to add "--rom-support" and "--rom-auto-lightfunc".