/*
 *  Master start for Web IDE.   Here we run the bootwifi script to boot up the WiFi
 *  environment.  Then we load the ide_webserver and we are off and running.
 */
var bootwifi = require("bootwifi.js");

// Boot WiFi and, when done, start the IDE WebServer.
bootwifi(function() {
	log("start.js: bootwifi callback arrived ... starting ide");
	var ide_webserver = require("ide_webserver.js");
	log("About to start ide_webserver()");
	ide_webserver();
}); // bootwifi