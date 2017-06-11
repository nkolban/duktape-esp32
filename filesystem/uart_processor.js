/*
 * Process incoming UART commands that arrive in HTTP format.
 * The command to be executed is supplied in the HTTP header in the
 * X-Command header property.  The supported values are:
 * 
 * * RUN  - Run the body as a script.
 * * SAVE - Save the body as a file.
 * * LIST - List the files on the file systems.
 * * DISABLESTART - Disable the auto start of the module.
 * 
 * The serial port access is provided by the serial class.
 */

/* globals require, log, module, DUKF */
(function() {
	var parserStreamWriter;
	var serialPort;
	function createHTTPParser() {
		var accumulatedData = "";
		var fs;
		parserStreamWriter = new HTTPParser(HTTPParser.REQUEST, function(parserStreamReader){
			log("We are ready to start parsing");
			parserStreamReader.on("data", function(data) {
				log("We receieved parsed data.  Length: " + data.length + ", data: " + data);
				accumulatedData += data;
			});
			
			// parsing of HTTP completed.  Now we process the commands
			parserStreamReader.on("end", function() {
				log("Parsing completed");
				log("Our accumulated data is: " + accumulatedData);
				if (parserStreamReader.headers.hasOwnProperty("X-Command")) {
					var command = parserStreamReader.headers["X-Command"].toUpperCase();
					log("Command is: " + command);
					if (command == "RUN") {
						try {
							eval(accumulatedData);
						} catch(e) {
							log("Exception caught: " + e.stack);
						}
					} // End of RUN
					if (command == "SAVE") {
						// At this point the data to save is in the accumulated data and what we need to
						// do now is save that in  a file.
						if (!parserStreamReader.headers.hasOwnProperty("X-Filename")) {
							log("No target file name supplied");
						} else {
							fs = require("fs");
							var fileName = parserStreamReader.headers["X-Filename"];
							fs.createWithContent("/spiffs" + fileName, accumulatedData);
							if (parserStreamReader.headers["X-Start"] === "true") {
								DUKF.setStartFile(fileName);
							}
						}
					} // End of Save
					if (command == "LIST") {
						fs = require("fs");
						var fileList = fs.spiffsDir();
						log(JSON.stringify(fileList));
						serialPort.write(JSON.stringify(fileList));
						serialPort.write("\r\n");
					} // End of List
					
					if (command == "DISABLESTART") {
						var NVS = require("nvs");
						var esp32duktapeNS = NVS.open("esp32duktape", "readwrite");
						esp32duktapeNS.erase("start");
						esp32duktapeNS.commit();
						esp32duktapeNS.close();
						log("Start of program at boot now disabled");
					}
				}
				createHTTPParser();
			});
		});
	} // createHTTPParser
	
	var HTTPParser = require("httpparser");
	var PORT = 2;
	var BAUD = 19200;
	var Serial = require("Serial");
	serialPort = new Serial(PORT);
	serialPort.configure({
		//baud: 115200,
		baud: BAUD,
		rxBufferSize: 1024,
		rxPin: 16,
		txPin: 17
	});
	
	createHTTPParser();
	
	serialPort.on("data", function(data) {
		parserStreamWriter.write(data);
	});
	log("UART command processor listening on UART " + PORT + "at a baud of " + BAUD);
})();

// Nothing to export.  This module runs and sets itself up to execute in the
// background.
module.exports = null;