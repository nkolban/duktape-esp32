var GPIO = require("gpio.js");
var pin = new GPIO(25);
pin.setPullMode(GPIO.PULLDOWN_ONLY);
pin.setInterruptHandler(GPIO.INTR_POSEDGE, function(pin) {
	log("The pin went high!: " + pin);
});