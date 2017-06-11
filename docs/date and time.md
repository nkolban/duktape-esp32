# Date and time
Since date and time are OS specific concepts, Duktape wants the provider to map to the
underlying technology to be used.  This is achieved by creating a function in C that returns
the current milliseconds since the epoch (Jan 1, 1970).  The signature of the function is:

```
duk_double_t get_now() {
   ...
   return value;
}
```

In our project, we implemented:

```
duk_double_t esp32_duktape_get_now() {
   struct timeval tv;
   gettimeofday(&tv, NULL);
   duk_double_t ret = floor(tv.tv_sec * 1000 + tv.tv_usec/1000);
   return ret;
} // esp32_duktape_get_now

```

To tell Duktape which function to use, we edit `duk_config.h` and add:

```
#define DUK_USE_DATE_GET_NOW(ctx) esp32_duktape_get_now()
#define DUK_USE_DATE_GET_LOCAL_TZOFFSET(d)  0
```

See also:

* [https://github.com/svaarala/duktape/blob/master/doc/datetime.rst](https://github.com/svaarala/duktape/blob/master/doc/datetime.rst)