var NVS = require("nvs");
var esp32duktapeNS = NVS.open("esp32duktape", "readwrite");
if(false) {
	esp32duktapeNS.set("NetVFS_on", 1, "uint8");
	var addr = "192.168.10.1";
//addr = ""; // uncomment this to deactivate NetVFS
	esp32duktapeNS.set("NetVFS_addr", addr, "string");
	esp32duktapeNS.set("NetVFS_port", 35555, "int");
	esp32duktapeNS.commit();
}
var NetVFS_on = esp32duktapeNS.get("NetVFS_on", "uint8");
var NetVFS_addr = esp32duktapeNS.get("NetVFS_addr", "string");
var NetVFS_port = esp32duktapeNS.get("NetVFS_port", "int");
log('NetVFS_on = ' + NetVFS_on);
log('NetVFS_addr = ' + NetVFS_addr);
log('NetVFS_port = ' + NetVFS_port);
esp32duktapeNS.close();


var Serial = require("Serial");
var serialPort = new Serial(1);
serialPort.configure({
	baud: 115200,
	rxPin: 16,
	txPin: 17
});
serialPort.write("app/main.c: Hello World, hit any key to reboot.\r\n");
serialPort.on('data', function(d) {log('Reboot');ESP32.reboot();});

var test = require('./test.js');
