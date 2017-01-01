// Write data to Serial port 0
//
var Serial = require("Serial");
var serialPort = new Serial(0);
serialPort.configure({
	baud: 115200
});
serialPort.write("Hello World");