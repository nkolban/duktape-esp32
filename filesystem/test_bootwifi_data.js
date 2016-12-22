var NVS  = require("nvs.js");
var bootWiFi_ns = NVS.open("bootwifi", "readwrite");

// Retrieve any saved NVS values.
var ssid     = bootWiFi_ns.get("ssid", "string");
var password = bootWiFi_ns.get("password", "string");
var ip       = bootWiFi_ns.get("ip", "string");
var gw       = bootWiFi_ns.get("gw", "string");
var netmask  = bootWiFi_ns.get("netmask", "string");
console.log("Bootwifi settings\n");
console.log("ssid: " + ssid + "\n");
console.log("password: " + password + "\n");
console.log("ip: " + ip + "\n");
console.log("gw: " + gw + "\n");
console.log("netmask: " + netmask + "\n");

var setValues = false;
if (setValues) {
	bootWiFi_ns.set("ssid", "sweetie", "string");
	bootWiFi_ns.set("password", "", "string");
	bootWiFi_ns.set("ip", "192.168.1.99", "string");
	bootWiFi_ns.set("gw", "192.168.1.1", "string");
	bootWiFi_ns.set("netmask", "255.255.255.0", "string");

}
bootWiFi_ns.close();