/*
 * AES module.
 */

/* globals ESP32, log, module, LOGE, Buffer, cancelInterval */


var moduleAES = ESP32.getNativeFunction("ModuleAES");
if (moduleAES === null) {
	log("Unable to find ModuleAES");
	module.exports = null;
	return;
}

var internalAES = {};
moduleAES(internalAES);
internalAES.init();


module.exports = internalAES;

