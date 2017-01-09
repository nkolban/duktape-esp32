/*
 * RMT module.
 * 
 * The remote module is able to generate complex signals.
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