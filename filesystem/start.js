/*
 *  Master start for Web IDE.   Here we run the bootwifi script to boot up the WiFi
 *  environment.  Then we load the ide_webserver and we are off and running.
 */
function startIDE() {
	log("start.js: bootwifi callback arrived ... starting ide");
	var ide_webserver = require("ide_webserver.js");
	log("About to start ide_webserver()");
	ide_webserver();
};


// Boot WiFi and, when done, start the IDE WebServer.
if (DUKF.OS == "ESP32") {
	// Check to see if we are booting serial ...
	var NVS = require("nvs");
	var esp32duktapeNS = NVS.open("esp32duktape", "readwrite");
	var useSerial = esp32duktapeNS.get("useSerial", "uint8");
	esp32duktapeNS.close();
	if (useSerial === 1) {
		require("uart_processor");
	} else {
		var bootwifi = require("bootwifi.js");
		bootwifi(startIDE);
	}
} else {
	startIDE();
}