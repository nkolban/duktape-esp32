/*
 * Module: gpio
 * 
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
		// Set the direction of the GPIO pin to either INPUT or OUTPUT.
		//
		setDirection: function(direction) {
			currentDirection = direction;
			OS.gpioSetDirection(pinNumber, direction);
		},
		//
		// setLevel
		// Set the signal level of the GPIO to be HIGH or LOW.
		//
		setLevel: function(level) {
			OS.gpioSetLevel(pinNumber, level);
		},
		//
		// getLevel
		// Get the current signal level of the GPIO.
		//
		getLevel: function(level) {
			return OS.gpioGetLevel(pinNumber);
		},
	};
	ret.setDirection(gpio.INPUT); // Set the initial pin direction as Input
	return ret;
} // gpio


// Set constants on the function.
gpio.INPUT = 0;   // Set direction to INPUT
gpio.OUTPUT = 1;  // Set direction to OUTPUT
gpio.HIGH = true; // HIGH signal
gpio.LOW = false; // LOW signal
module.exports = gpio;