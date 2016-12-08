#Event processing
Within ESP32-Duktape, there is a primary JavaScript loop which runs code.  In addition there
are a variety of asynchronous tasks running thanks to FreeRTOS, Interrupt Service Routines (ISRs) and
WiFi/network activity.  It is vital that these asynchronous routines NOT attempt to work with the JS environment
while the primary thread of JS is running.  Instead, these routines must post an event that contains the data
that we want to pass into JS.  In the JS main loop, when we have nothing else to do, we examine the event queue
and if there is something waiting our attention, we process it there.

But what about contextual data?  Let me try and explain this.  Imagine we have a JavaScript function
that is of the form:

`doSomethingAync(callback, object)`

This should be read as "Hey, ESP32 ... initiate something in the background, when done, invoke the callback
and pass the object data.  Since, by definition, `doSomethingAsync` will not return in real time, we need to 
save information away such that when that something happens, we invoke the callback with the data.  However, we
can't simply use singleton globals.  What if I invoke:

```
doSomethingAync(thing1, callback1, object1);
doSomethingAync(thing2, callback2, object2);
```

We want to associate the async response on `thing1` against `callback1` and `object1`.  It would be wrong for
the response on `thing1` to be sent to `callback2` or with `object2`.

In Duktape, to prevent an object from being garbage collected, it must be accessible from the list of objects.  This
means we need to keep a "path" to it.  In addition, we will need some form of correlator to correlate a
request, an async response and the actual callback/data for THAT response.

#Callback processing
Imagine that we ask the C environment to watch for something at the C level and asynchronously call a function
when it sees what it is looking for.  As a concrete example, imagine the sockets `accept()` call.  This is blocking
call that waits for a new client connection.  If we wanted to implement this in JS, we couldn't simply call:

```
// JavaScript
var newSocket = socket.accept();

```

because it would block.  Instead we have to do something like:

```
// JavaScript
socket.accept(function(newSocket) {
   // Process newSocket
});
```

Ok ... that seems to make sense.  So what it appears we need is a C function that is called from the JS
environment that takes a callback function as a parameter and, when the something that the C function
is interested in happens, call that JS callback.

