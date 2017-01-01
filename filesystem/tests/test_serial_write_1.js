// Write data to Serial port 1
//
var Serial = require("Serial");
Serial.configure(2, {
	baud: 115200,
	rxPin: 16,
	txPin: 17
});
Serial.write(2, "Hello World");