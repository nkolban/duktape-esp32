(function() {
	
var STOP_PIN = 13; 
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


/*
 * Determine if we should run a start program, what it should be and then
 * actually run it if needed.
 */
function runStart() {
	var GPIO = require("gpio");
	var stopPin = new GPIO(STOP_PIN);
	stopPin.setDirection(GPIO.INPUT);
	if (stopPin.getLevel()) {
		log("Start program bypassed by user.");
		return;
	}
	
	var esp32duktapeNS = NVS.open("esp32duktape", "readwrite");
	var startProgram = esp32duktapeNS.get("start", "string");
	esp32duktapeNS.close();
	if (startProgram !== null) {
		log("Running start program: %s" + startProgram);
		var script = ESP32.loadFile("/spiffs" + startProgram);
		if (script !== null) {
			try {
				eval(script);
			}
			catch (e) {
				log("Caught exception: " + e.stack);
			}
		} // We loaded the script
	} else { // We have a program 
		log("No start program");
	}
} // runStart


// Boot WiFi and, when done, start the IDE WebServer OR
// Boot the uart_processor
if (DUKF.OS == "ESP32") {
	// Check to see if we are booting serial ...
	var NVS = require("nvs");
	var esp32duktapeNS = NVS.open("esp32duktape", "readwrite");
	var useSerial = esp32duktapeNS.get("useSerial", "uint8");
	log("useSerial: " + useSerial);
	esp32duktapeNS.close();
	if (useSerial === 1) {
	//if (useSerial === 1 || useSerial === null) {
		log("+----------------+");
		log("| uart_processor |");
		log("+----------------+");
		require("uart_processor");
		runStart();
	} else {
		var bootwifi = require("bootwifi.js");
		bootwifi(startIDE);
	}
} else {
	startIDE();
}

})();