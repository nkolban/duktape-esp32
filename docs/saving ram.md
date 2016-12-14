#Saving RAM
While the ESP32 has a whopping 512K RAM, apparently a maximum on about 380K can be used as data ram (heap) and experience
is showing that we start with about 150K.  Take away that needed by basic Duktape and other goodies and we are left
with only about 70K for our apps ... and this dwindles fast.  As such, we need to look at ways in which we
can reduce RAM utilization.  Note that it appears that we are not short on Flash, so ROM storage is not a concern.

An excellent paper on Duktape and RAM utilization can be found here

[https://github.com/svaarala/duktape/blob/master/doc/low-memory.rst](https://github.com/svaarala/duktape/blob/master/doc/low-memory.rst)


DUK_OPT_LIGHTFUNC_BUILTINS
