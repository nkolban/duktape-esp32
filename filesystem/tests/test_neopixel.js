/*
 * Test driving Neopixels.
 * Attach the data-in of the Neopixels to the GPIO pin.
 */
//ESP32.setLogLevel("*", "error");
var PIN = 25;
//var PIN = 21;
var MAX = 16;
var PIXEL_COUNT = 16;

var neopixel = require("neopixels");
var n = new neopixel(PIN, PIXEL_COUNT);
setInterval(function() {
	var i;
	var red, green, blue;
	n.clear();
	red = Math.floor(Math.random() * 256);
	green = Math.floor(Math.random() * 256);
	blue = Math.floor(Math.random() * 256);
	red = 0;
	green = 1;
	blue = 0;
	for (i=0; i<PIXEL_COUNT && i<MAX; i++) {
		n.setPixel(i, red, green, blue);
	}
	n.show();
}, 1000);

