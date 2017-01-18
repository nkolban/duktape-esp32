// Node.js UART script runner
// Read a JS script to run and send it to the ESP32 over a UART connection.
var fs = require("fs");
var SerialPort = require("serialport");
var argv = require("minimist")(process.argv.slice(2), {
	string: "f"
});
if (argv.hasOwnProperty("h")) {
	console.log("Usage: -f <script file>");
	return;
}
if (!argv.hasOwnProperty("f")) {
	console.log("No file supplied");
	return;
}
var fileName = argv.f;
var fileData = fs.readFileSync(fileName);
//console.log("File content is: " + fileData);

var port = new SerialPort("/dev/ttyUSB1", {
	//baudRate: 115200
	baudRate: 19200
});
port.on("open", function() {
	//console.log("Ready to write down the serial port!");
	port.write("POST /run HTTP/1.1\r\n");
	port.write("Content-Length: " + fileData.length + "\r\n");
	port.write("X-Command: run\r\n");
	port.write("\r\n");
	port.write(fileData, function() {
		port.close(); // When the file data has been written, that's the end so we can close the port.
		//console.log("Done: The content of file " + fileName + " has been sent.");
	});
});
port.on("error", function(err) {
	console.log("Error: " + err.message);
});
