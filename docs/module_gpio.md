# GPIO module
This module provides access to the GPIO functions.

##Example - Blinky
In this example, we connect an LED to GPIO 18 and blink it once a second.

```
var GPIO = require("gpio.js");
var pin = new GPIO(18);
var level = true;
pin.setDirection(GPIO.OUTPUT);
setInterval(function() {
	pin.setLevel(level);
	level = !level;
}, 1000);
```