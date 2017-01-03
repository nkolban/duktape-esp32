/*
 * Process incoming UART commands that arrive in HTTP format.
 * The command to be executed is supplied in the HTTP header in the
 * X-Command header property.  The supported values are:
 * 
 * * RUN - Run the body as a script.
 */

/* globals require, log, module */
(function() {
	var parserStreamWriter;
	function createHTTPParser() {
		var accumulatedData = "";
		parserStreamWriter = new HTTPParser(HTTPParser.REQUEST, function(parserStreamReader){
			log("We are ready to start parsing");
			parserStreamReader.on("data", function(data) {
				log("We receieved parsed data.  Length: " + data.length + ", data: " + data);
				accumulatedData += data;
			});
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
					}
				}
				createHTTPParser();
			});
		});
	}
	
	var HTTPParser = require("httpparser");
	var PORT = 2;
	var Serial = require("Serial");
	var serialPort = new Serial(PORT);
	serialPort.configure({
		//baud: 115200,
		baud: 19200,
		rxBufferSize: 1024,
		rxPin: 16,
		txPin: 17
	});
	
	createHTTPParser();
	
	serialPort.on("data", function(data) {
		parserStreamWriter.write(data);
	});
	log("UART command processor listening on UART " + PORT);
})();

module.exports = null;