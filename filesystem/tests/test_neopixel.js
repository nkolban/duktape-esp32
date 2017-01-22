/*
 * Test driving Neopixels.
 * Attach the data-in of the Neopixels to the GPIO pin.
 */
ESP32.setLogLevel("*", "error");
log("Free heap: " + ESP32.getState().heapSize);
var PIN = 21;
var MAX = 20;
var PIXEL_COUNT = 20;

var neopixel = require("neopixels");
var n = new neopixel(PIN, PIXEL_COUNT);
setInterval(function() {
	var i;
	var red, green, blue;
	n.clear();
	red = Math.floor(Math.random() * 256);
	green = Math.floor(Math.random() * 256);
	blue = Math.floor(Math.random() * 256);
	//red = 0;
	//green = 1;
	//blue = 0;
	for (i=0; i<PIXEL_COUNT && i<MAX; i++) {
		n.setPixel(i, red, green, blue);
	}
	log("Free heap: " + ESP32.getState().heapSize);
	n.show();
}, 100);

