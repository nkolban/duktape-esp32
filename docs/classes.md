#Modules
Built into the solution are a variety of Modules.

##Globals
These are functions that are in the global environment.

###cancelInterval
Cancel an interval timer created by a call to `setInterval()`.
Syntax:
`cancelInterval(id)`

The `id` is a value returned from a previous call to `setInterval()`.  The function associated with the timer will
no longer be fired.  It may not even fire once if the initial interval since it was started is not reached.

###cancelTimeout
Cancel a timer created by a call to `setTimeout()`.
Syntax:
`cancelTimeout(id)`

The `id` is a value returned from a previous call to `setTimeout()`.  If the timer has not expired, the function
associated with the timer will not be called.
 
###setInterval
Set a repeating timer to call a function periodically.

Syntax:
`setTimer(function, interval)`

The `interval` is the period in milliseconds after which the function will be called.  When the function
completes, the timer will be restarted using the same interval.
The `function` is a function which will be invoked when the timer fires.

`setTimer` returns an id which can be used through a call to `cancelTimer()` to cancel any subsequent fires of the timer.

###setTimeout
Fire a single shot timer.

Syntax:
`setTimeout(function, delay)`

The `delay` is a time in milliseconds after which the function will be called.  This is a single shot timer and
once the function has executed, it will not be scheduled for re-execution.
The `function` is a function which will be invoked after the delay interval.

`setTimeout` returns an id which can used through a call to `cancelTimeout()` to cancel a timer before
it has fired.

##console
###log
Log a text string to the console.

Syntax:
`console.log(string)`

##HTTP
###request
Make an HTTP request.

Syntax:

`request(options, [callback])`



##ESP32
###getNativeFunction
Retrieve a built-in native function by name.

Syntax:

`getNativeFunction(name)`

Returns a function reference or null if not found.

###getState
Retrieve the state of the ESP32 device.

Syntax:

`getState()`

This returns an object which contains:
```
{
   heapSize - The size of the free heap.
   sdkVersion - The version of the ESP32 SDK we are running upon.
}
```

###load
Load a JavaScript source file from the network and run it.

Syntax:

`load(url)`

###reset
Reset the state of the JavaScript environment.

Syntax:

`reset()`



##WiFi
###disconnect
Disconnect from the currently connected access point when we are a station.

Syntax:

`disconnect()`

###getDNS
Retrieve an array of the two IP addresses (as strings) that represent the two
DNS servers that we may know about.

Syntax:

`getDNS()`


###getState
Retrieve the current state of the WiFi environment.

Syntax:
`getState()`

This returns an object which contains:
```
{
   isStation - True if we are being a station and false otherwise.
   isAcessPoint - True if we are being an access point and false otherwise.
   apMac - The MAC address of our access point interface.
   staMac - The MAC address our station interface.
   country - The country code in effect.
}
```


###scan
Perform an access point scan and invoke a callback when complete.

Syntax:

`scan(callback)`

The `callback` is an array of objects where each object contains:

```
{
   ssid - The SSID of the network.
   mac - The MAC address of the access point.  Format is xx:xx:xx:xx:xx:xx.
   rssi - Signal strength.
   auth - One of "open", "wep", "wpa", "wpa2", "wpa_wpa2"
}
```

Example:

```
var wifi = require("WiFi");
wifi.scan(function(list) {
   print(JSON.stringify(list));
});
```

##PARTITIONS
The `PARTITIONS` object provides access to the ESP32 partition table.

###list
Retrieve the details of a partition table type.

Syntax:
`list(type)`

The `type` is the type of partition table set to return.  The choices are "app" and "data".  The return value
is an array of object where each object contains:

```
{
   "type":    <String> - Type of partition.
   "subtype": <String> - Subtype of partition.
   "label":   <String> - Label of partition.
   "address": <number> - Address in flash of partition start.
   "size":    <number> - Length of partition (bytes).
}
``` 

##RMT
###getState
Retrieve the state of a give channel.

Syntax:

`getState(channel)`

An object is returned which contains the state of the channel as understood by the ESP32:
```
{
   channel: <number> - The channel id.
   loopEnabled: <boolean> - Whether or not the TX loop is enabled.
   clockDiv: <number> - The clock divisor.
   memBlocks: <number> - The memory blocks being used.
   idleLevel: <number> - The idle level.
   memoryOwner: <String> - The memory owner "TX" or "RX"
}
```

###txConfig
Configure a RMT channel for transmission.

Syntax:

`txConfig(channel, options)`

The options is an object with the following properties:

```
{
   gpio: <number> - GPIO pin to use.
   memBlocks: <number> - Memory blocks to use. [Default=1]
   idleLevel: <boolean> - Idle value to use. [Default=false]
   clockDiv: <number> - Clock divider. [Default=1]
 }
 ```
 
###write
Write items into the output stream.  We wait until the output has completed.

Syntax:
 
`write(channel, arrayOfItems)`
 
The arrayOfItems is an array of item objects where each item object contains
```
{
   level: <boolean> - The output signal level.
   duration: <number> - The duration of this signal.
}
```

