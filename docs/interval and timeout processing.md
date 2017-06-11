# Interval and timeout processing
Consider the JavaScript functions `setInterval()` and `setTimeout()`.  Surprisingly to many folks, these are
not in fact part of the JavaScript language but are instead environmentally provided.

The `setTimeout()` call registers a function to be invoked some milliseconds in the future while
`setInterval()` does the same but when the timeout occurs, schedules it again for restart at the
next interval.

The high level design for these functions is similar.  We will assume a record which contains:

* `func` - function to invoke
* `duration` - duration to wake up after registration
* `interval` - a boolean to indicate if this is an interval or a timeout
* `wakeTime` - The milliseconds value from the epoch at which to wake up
* `id` - an id for this timer request

Now imagine that these records exist in a sorted list sorted by the wakeTime with the
sooner time first.  With this in mind, we can now create a single task which works with the following
psuedo code:

```
for(;;) {
   if (the list is not empty) {
      get the first entry from the list;
      if (now >= wakeTime) {
         run the function (by posting a run now event);
         remove the head of the list.
      } else {
         calculate the sleepTime as wakeTime of first entry on list - now;
         wake when the list changes or the sleepTime has expired
      }
   } else {
      wake when the list changes (no wake on timer);
   }
}
```

This assumes that we have a way to sleep for a period of time
or until the list changes.