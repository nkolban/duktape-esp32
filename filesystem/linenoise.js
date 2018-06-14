/*
 * Linenoise module.
 */

/* globals ESP32, log, module, LOGE, Buffer, cancelInterval */


var moduleLinenoise = ESP32.getNativeFunction("ModuleLinenoise");
if (moduleLinenoise === null) {
	log("Unable to find ModuleLinenoise");
	module.exports = null;
	return;
}

var internalLinenoise = {};
moduleLinenoise(internalLinenoise);
internalLinenoise.init();

module.exports = internalLinenoise;
