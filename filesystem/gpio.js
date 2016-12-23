/*
 * Module: gpio
 * Load with:
 * 
 * require("gpio.js")
 * 
 * Example snippet:
 * ----
 * var GPIO = require("gpio");
 * var pin1 = new GPIO(1);
 * pin1.setDirection(GPIO.OUTPUT);
 * pin1.setLevel(GPIO.HIGH);
 * ----
 */
function gpio(pinNumber) {
	OS.gpioInit(pinNumber);
	var currentDirection;
	var ret = {
		//
		// setDirection
		//
		setDirection: function(direction) {
			currentDirection = direction;
			OS.gpioSetDirection(pinNumber, direction);
		},
		//
		// setLevel
		//
		setLevel: function(level) {
			OS.gpioSetLevel(pinNumber, level);
		},
		//
		// getLevel
		//
		getLevel: function(level) {
			return OS.gpioGetLevel(pinNumber);
		},
	};
	ret.setDirection(gpio.INPUT); // Set the initial pin direction as Input
	return ret;
} // gpio

gpio.INPUT = 0;
gpio.OUTPUT = 1;
gpio.HIGH = true;
gpio.LOW = false;
module.exports = gpio;