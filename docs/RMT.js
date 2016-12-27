/*
 * RMT module.
 */
var moduleRMT = ESP32.getNativeFunction("ModuleRMT");
if (moduleRMT === null) {
	log("Unable to find ModuleRMT");
	module.exports = null;
	return;
}

var rmt = {	
};

moduleRMT(rmt);

module.exports = rmt;