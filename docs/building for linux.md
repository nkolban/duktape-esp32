# Building for Linux
There may be times when you want to play with Duktape outside of the ESP32 environment.
Here is a recipe:

1. Make a directory called duktape
```
$ mkdir duktape
$ cd duktape
```

2. Download the latest distribution
```
$ wget http://duktape.org/duktape-1.5.1.tar.xz
$ tar -xvf http://duktape.org/duktape-1.5.1.tar.xz
$ cd duktape-1.5.1

```

3. Build duktape
```
$ make -f Makefile.cmdline
```

At this point you will have the `duk` binary ready to run.

