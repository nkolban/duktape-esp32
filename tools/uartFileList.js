
/*
 * Node.js UART file list 
 * Dump the list of files on the ESP32
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
	port.write("GET /list HTTP/1.1\r\n");
	port.write("X-Command: list\r\n");
	port.write("\r\n");
});
port.on("data", function(data) {
	console.log("We received: " + data);
	var fileList = JSON.parse(data);
	fileList.sort(function(a, b) {
		return a.name.localeCompare(b.name);
	});
	var i;
	for (i=0; i<fileList.length; i++) {
		console.log(fileList[i].name);
	}
	port.close();
});
port.on("error", function(err) {
	console.log("Error: " + err.message);
});
