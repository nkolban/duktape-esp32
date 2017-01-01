// Write data to Serial port 2
//
var port = 2;
var data = new Buffer(100);
var Serial = require("Serial");
var serialPort = new Serial(2);
serialPort.configure({
	baud: 115200,
	rxPin: 16,
	txPin: 17
});
serialPort.write("Hello World");
serialPort.on("data", function(data) {
	log("We got some data! " + data.length);
});
