// Write data to Serial port 0
//
var Serial = require("Serial");
Serial.configure(0, {
	baud: 115200
});
Serial.write(0, "Hello World");