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
	var NetVFS_on = esp32duktapeNS.get("NetVFS_on", "uint8");
	log("NetVFS_on: " + NetVFS_on);
	var useSerial = esp32duktapeNS.get("useSerial", "uint8");
	log("useSerial: " + useSerial);
	if(false) { // serial VFS filesystem (use tools/SerialVFSServer.js)
		var Serial = require("Serial");
		var num = 2;
		var sport = new Serial(num);
		sport.configure({
			baud: 115200*2,
			rxPin: 4, // GPIO4 for WROVER
			txPin: 5, // GPIO5 for WROVER
			rxBufferSize: 1024
		});
		var dir = 'app';
		var ttt = ESP32.getNativeFunction("ModuleSerialVFS");
		var internalSerialVFS = {};
		ttt(internalSerialVFS);
		internalSerialVFS.init({mount: '/' + dir, serial: num});
		ESP32.NetVFSDir = dir;
		log('SerialVFS done');
		setTimeout(function() {require(dir + '/main.js');}, 10);
	} else if(NetVFS_on) {
		log('NetVFS boot');
		var NetVFS_addr = esp32duktapeNS.get("NetVFS_addr", "string");
		var NetVFS_port = esp32duktapeNS.get("NetVFS_port", "int");
		var dir = 'app';
		var bootwifi = require("bootwifi.js");
		bootwifi(function () {
			log('wifidone!');
			if(NetVFS_addr) {
				ESP32.NetVFSDir = dir;
				var ttt = ESP32.getNativeFunction("ModuleNetVFS");
				var internalNetVFS = {};
				ttt(internalNetVFS);
				internalNetVFS.init({mount: '/' + dir, server:NetVFS_addr, port:NetVFS_port});
			}
			require(dir + '/main.js');
			log('NetVFS done');
		});
	} else if (useSerial === 1) {
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
	esp32duktapeNS.close();
} else {
	startIDE();
}

})();