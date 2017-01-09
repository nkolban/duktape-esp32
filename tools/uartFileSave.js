/*
 * Node.js UART file save runner
 * Read a file and save it as a file on the ESP32.
 */

/* globals require, process */
var fs = require("fs");
var SerialPort = require("serialport");
var argv = require("minimist")(process.argv.slice(2), {
	string: "ft", // The -f flag means source file, the -t flag means target file.
	boolean: "s" // The -s flag means save.
});
if (!argv.hasOwnProperty("f")) {
	console.log("No source file supplied");
	return;
}
if (!argv.hasOwnProperty("t")) {
	console.log("No target file supplied");
	return;
}
var sourceFileName = argv.f;
var targetFileName = argv.t;
var flagSave = argv.s;
var fileData = fs.readFileSync(sourceFileName);
//console.log("File content is: " + fileData);

var port = new SerialPort("/dev/ttyUSB1", {
	//baudRate: 115200
	baudRate: 19200
});
port.on("open", function() {
	//console.log("Ready to write down the serial port!");
	port.write("POST /save HTTP/1.1\r\n");
	port.write("Content-Length: " + fileData.length + "\r\n");
	port.write("X-Command: save\r\n");
	port.write("X-Filename: " + targetFileName + "\r\n");
	if (flagSave) {
		port.write("X-Start: true\r\n");
	}
	port.write("\r\n");
	port.write(fileData, function() {
		port.close(); // When the file data has been written, that's the end so we can close the port.
		//console.log("Done: The content of file " + fileName + " has been sent.");
	});
});
port.on("error", function(err) {
	console.log("Error: " + err.message);
});
