/*
 * Script to manipulate the NVS values used by uart_processor.  The script
 * dumps the contents and then can optionally:
 * 
 * * Set the values.
 * * Erase all the values.
 * 
 */
var NVS  = require("nvs.js");
var esp32duktape_ns = NVS.open("esp32duktape", "readwrite");

// Retrieve any saved NVS values.
var useSerial = esp32duktape_ns.get("useSerial", "uint8");
var startProgram = esp32duktape_ns.get("start", "string");
log("esp32duktape settings");
log("useSerial: " + useSerial);
log("startProgram: " + startProgram);


var setValues = false;
if (setValues) {
	esp32duktape_ns.set("useSerial", 1, "uint8");
	esp32duktape_ns.commit();
}

var eraseAll = false;
if (eraseAll) {
	esp32duktape_ns.eraseAll();
}

esp32duktape_ns.close();