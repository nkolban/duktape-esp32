var NVS = require("nvs");
function doAccessPoint() {
	WIFI.listen({
		ssid: "esp32-duktape",
		auth: "open"
	}, function() {
		log("Wifi Callback!!!");
	});
}
// Get the SSID and password to connect to the access point

var bootWiFi_ns = NVS.open("bootwifi", "readwrite");
try {
	var ssid = bootWiFi_ns.get("ssid", "string");
	var password = bootWiFi_ns.get("password", "string");
}
catch(e) {
	log("caught: " + e);
}
bootWiFi_ns.close();
// Startup as a WiFi access point
doAccessPoint();
// Attempt to connect to the access point
log("ESP32 Heap: " + ESP32.getState().heapSize);