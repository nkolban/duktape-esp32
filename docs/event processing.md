# Event processing
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

# Callback processing
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
is interested in happens, call that JS callback.  The first thing we have to consider is that the
JS environment will pass (to C) a callback function that is to be invoked at a later time.  It is vital
that the callback function be stashed.  Let us examine this and see why we need to stash it.

Consider the following JS code
```
// JS
callCFunc(function(params) {
   // do something
});
```
Notice that the callback function is defined as anonymous and in-line.  When this statement is reached, a new
anonymous function is created and passed in as a parameter to the function called `callCFunc`.  When `callCFunc`
returns, the anonymous function is eligible for garbage collection.  If at some time later, the C code that wishes
to call the anonymous function (the callback) attempts to do so, any handle it may have simply had may no
longer be valid as it was cleaned up.  To resolve this issue, the C code must ask JS to keep a reference to the
function and that is what we call stashing.  If we have stashed the function, then when the C code wishes to
call the JS in the future, it can retrieve the stashed handle and call it.

Our framework provides a number of functions for stashing.

First there is `esp32_duktape_stash_object`.

```
uint32_t esp32_duktape_stash_object(duk_context *ctx)
```
This takes the object at the top of the stack and places it
in a new property  in the stash.  A key is returned that we can later use to get the data back.
The next function we look at is called `esp32_duktape_unstash_object`:
```
void esp32_duktape_unstash_object(duk_context *ctx, uint32_t key)
```
This function takes a key previously generated/returned by `esp32_duktape_stash_object` and pushes a single object on the
value stack.  That object contains the item previously stashed.  Note that the stashed item is NOT deleted.
It still remains in the stash.  To delete a stashed entry we call:

```
void esp32_duktape_stash_delete(duk_context *ctx, uint32_t key)
```

Passing in the generated/returned key that we got from `esp32_duktape_stash_object`.  Failure to delete
stashed objects over time will result in a bad memory leak.


The last thing we have to consider is that when a C event occurs, we are NOT allowed to work with JavaScript.  We simply have
no idea where we are in the JavaScript world we may be.  To handle this, what we must do is post an event to our
event loop so that when JavaScript is ready, it can pick it up.  Imagine that we have a C event handler that is
asynchronously called when some event happens:

```
void my_C_event_handler() {
   // We want to call the JS callback here ... but we are in the wrong context!!!
}
```

Instead we must code:
```
void my_C_event_handler() {
   event_newCallbackRequestedEvent(
      ESP32_DUKTAPE_CALLBACK_TYPE_FUNCTION,
      stashKey, // Stash key for stashed callback array
      dataProvider, // Data provider parameter
      NULL // Context parameter
   );
}
```

Obviously we have some explaining to do here.  By calling `event_newCallbackRequestedEvent()` we are saying that
we are posting a new event that a request to callback JS is needed.

There might be multiple "styles" for invoking JS callbacks so we provide a parameter that defines the callback style we
want.  Currently only `ESP32_DUKTAPE_CALLBACK_TYPE_FUNCTION` is provided so this will be constant in our story.
Next comes `stashKey` which is a key to a previously stashed JS callback.  This would be stashed by `array` with the first
element in the array being the function to invoke.
The `dataProvider` is a C language function that is called to push additional values onto the Duktape call stack before
invoking `duk_pcall()` to perform the callback function.  This is our chance to push dynamic parameters to the
callback function.  The `dataProvider` is optional and can be `NULL` if we have no need to add dynamic parameters.
The signature of the `dataProvider` function is:

```
int dataProvider(duk_context *ctx, void *context)
```

The return value should be the number of new items pushed onto the value stack.  The `context` parameter is optional
and passed into the `dataProvider`.
