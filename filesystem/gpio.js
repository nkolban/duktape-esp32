/*
 * Module: gpio
 * 
 * Exposed functions:
 * * getLevel
 * * setDirection
 * * setInterruptHandler
 * * setLevel
 * * setPullMode
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

var moduleGPIO = ESP32.getNativeFunction("ModuleGPIO");
if (moduleGPIO === null) {
	log("Unable to find ModuleGPIO");
	module.exports = null;
	return;
}

var internalGpio = {};
moduleGPIO(internalGpio);

function gpio(pinNumber) {
	internalGpio.gpioInit(pinNumber);
	var currentDirection;
	var ret = {
		//
		// getLevel
		// Get the current signal level of the GPIO.
		//
		getLevel: function() {
			return internalGpio.gpioGetLevel(pinNumber);
		}, // getLevel
		
		//
		// setDirection
		// Set the direction of the GPIO pin to either INPUT or OUTPUT.
		//
		setDirection: function(direction) {
			currentDirection = direction;
			internalGpio.gpioSetDirection(pinNumber, direction);
		}, // setDirection
		
		//
		// setInterruptHandler
		// Set the interrupt handler for this GPIO.
		// intrType - One of:
		//  - INTR_ANYEDGE		
		//  - INTR_DISABLE
		//  - INTR_NEGEDGE		
		//  - INTR_POSEDGE
		// 
		setInterruptHandler: function(intrType, callback) {
			this.setDirection(gpio.INPUT);
			internalGpio.gpioISRHandlerRemove(pinNumber);
			internalGpio.gpioSetIntrType(pinNumber, intrType);
			if (intrType == gpio.INTR_DISABLE) {
				delete _isr_gpio[pinNumber];
			} else {
				_isr_gpio[pinNumber] = {
					callback: callback
				};
				internalGpio.gpioISRHandlerAdd(pinNumber);
			}
		}, // setInterruptHandler
		
		//
		// setLevel
		// Set the signal level of the GPIO to be HIGH or LOW.
		//
		setLevel: function(level) {
			internalGpio.gpioSetLevel(pinNumber, level);
		}, // setLevel
		
		//
		// setPullMode
		//
		setPullMode: function(mode) {
			return internalGpio.gpioSetPullMode(pinNumber, mode);
		}, // setPullMode
		

	}; // End ret
	ret.setDirection(gpio.INPUT); // Set the initial pin direction as Input
	return ret;
} // gpio


// Set constants on the function.
gpio.INPUT  = 0;     // Set direction to INPUT
gpio.OUTPUT = 1;     // Set direction to OUTPUT
gpio.HIGH   = true;  // HIGH signal
gpio.LOW    = false; // LOW signal
gpio.INTR_ANYEDGE = internalGpio.INTR_ANYEDGE;
gpio.INTR_DISABLE = internalGpio.INTR_DISABLE;
gpio.INTR_NEGEDGE = internalGpio.INTR_NEGEDGE;
gpio.INTR_POSEDGE = internalGpio.INTR_POSEDGE;

gpio.PULLUP_ONLY     = internalGpio.PULLUP_ONLY;
gpio.PULLDOWN_ONLY   = internalGpio.PULLDOWN_ONLY;
gpio.PULLUP_PULLDOWN = internalGpio.PULLUP_PULLDOWN;
gpio.FLOATING        = internalGpio.FLOATING;


internalGpio.gpioInstallISRService(0, function(pin) {
	if (_isr_gpio.hasOwnProperty(pin)) {
		_isr_gpio[pin].callback(pin);
	}
});
module.exports = gpio;