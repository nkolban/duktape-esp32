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

var bootwifi = require("bootwifi.js");

// Boot WiFi and, when done, start the IDE WebServer.
if (DUKF.OS == "ESP32") {
	bootwifi(startIDE);
} else {
	startIDE();
}