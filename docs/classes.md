#Modules
Built into the solution are a variety of Modules.

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
##getNativeFinction
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
   authMode - One of "open", "wep", "wpa", "wpa2", "wpa_wpa2"
}
```

Example:

```
var wifi = require("WiFi");
wifi.scan(function(list) {
   print(JSON.stringify(list));
});
```
