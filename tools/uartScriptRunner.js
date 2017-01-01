// Node.js UART script runner
// Read a JS script to run and send it to the ESP32 over a UART connection.
var fs = require("fs");
var argv = require("minimist")(process.argv.slice(2), {
	string: "f"
});
if (!argv.hasOwnProperty("f")) {
	console.log("No file supplied");
	return;
}
var fileData = fs.readFileSync(argv.f);
console.log("File content is: " + fileData);
var SerialPort = require("serialport");
var port = new SerialPort("/dev/ttyUSB1", {
	baudRate: 115200
});
port.on("open", function() {
	console.log("Ready to write down the serial port!");
	port.write("HTTP/1.1 200 OK\r\n");
	port.write("Content-Length: " + fileData.length + "\r\n");
	port.write("X-Command: run\r\n");
	port.write("\r\n");
	port.write(fileData);
	//port.close();
});
port.on("error", function(err) {
	console.log("Error: " + err.message);
});
