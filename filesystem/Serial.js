/*
 * Serial module.
 */
var moduleSerial = ESP32.getNativeFunction("ModuleSerial");
if (moduleSerial === null) {
	log("Unable to find ModuleSerial");
	module.exports = null;
	return;
}

var serial = {	
};

moduleSerial(serial);

module.exports = serial;