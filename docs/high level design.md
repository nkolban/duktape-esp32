# High level design
Here we discuss the high level design on the project.

```
Perform ESP32 initialization
Load the network subsystem (become a WiFi station - or an access point)
Perform Duktape initialization
Load the C modules
Load the "init.js" JavaScript file (loads the loop.js)
Load any script defined to run after core initialization.
while (true) {
   get next event;
   process next event;
   call JS Loop function;
}
```

And that's it.


## Event - JavaScript callback requested
Since we may only legally call JavaScript in a single thread, we can't call JavaScript from another thread.
This means that we can't *directly* invoke JavaScript when a hardware or network event is detected because
we may have performed a context switch while in the middle of JavaScript to service that interrupt.  As such
we queue a "JavaScript callback event" onto the queue indicating that we should invoke a specific JS callback
when the next opportunity presents itself.

## The JS Loop
JavaScript on the ESP32 is never idle and never goes to sleep.  Instead we execute a busy polling loop
called the "JS Loop function".   The secret sauce here is that the JS Loop is itself implemented
in JavaScript.  Only one iteration of the loop is ever invoked when called.  The loop
polls things that may need attention.  This includes:

* Expired timers
* Network socket accepts
* Network socket receives
* Network socket exceptions 