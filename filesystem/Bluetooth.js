/*
 * Bleutooth module.
 */

/* globals ESP32, log, module, LOGE, Buffer, cancelInterval */


var moduleBluetooth = ESP32.getNativeFunction("ModuleBluetooth");
if (moduleBluetooth === null) {
	log("Unable to find ModuleBluetooth");
	module.exports = null;
	return;
}

var internalBluetooth = {};
moduleBluetooth(internalBluetooth);
interalBluetooth.init();


module.exports = null;