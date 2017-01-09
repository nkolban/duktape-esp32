
/*
 * Node.js UART disable start at boot 
 * The access to the serial port is through the NPM module called "serialport":
 * https://www.npmjs.com/package/serialport
 * 
 */
var fs = require("fs");
var SerialPort = require("serialport");


var port = new SerialPort("/dev/ttyUSB1", {
	//baudRate: 115200,
	baudRate: 19200,
	//parser: SerialPort.parsers.raw
	parser: SerialPort.parsers.readline("\r\n")
});
port.on("open", function() {
	port.write("GET /disablestart HTTP/1.1\r\n");
	port.write("X-Command: DISABLESTART\r\n");
	port.write("\r\n", function() {
		port.close();
	});
});
port.on("error", function(err) {
	console.log("Error: " + err.message);
});
