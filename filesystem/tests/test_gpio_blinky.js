var GPIO = require("gpio.js");
var pin = new GPIO(18);
var level = true;
pin.setDirection(GPIO.OUTPUT);
setInterval(function() {
	pin.setLevel(level);
	level = !level;
}, 1000);