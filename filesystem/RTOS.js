/*
 * RTOS module.
 */
var moduleRTOS = ESP32.getNativeFunction("ModuleRTOS");
if (moduleRTOS === null) {
	log("Unable to find ModuleRTOS");
	module.exports = null;
	return;
}

var rtos = {	
};

moduleRTOS(rtos);

module.exports = moduleRTOS;