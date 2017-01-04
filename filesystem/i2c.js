/*
 * I2C module.
 * 
 * configureMaster:
 * o port num
 * o sda_pin
 * o scl_pin
 * o clk_speed
 */

/* globals ESP32, log, module, require */


var moduleI2C = ESP32.getNativeFunction("ModuleI2C");
if (moduleI2C === null) {
	log("Unable to find ModuleI2C");
	module.exports = null;
	return;
}

var internalI2C = {};
moduleI2C(internalI2C);

var i2c = {
};


module.exports = i2c;