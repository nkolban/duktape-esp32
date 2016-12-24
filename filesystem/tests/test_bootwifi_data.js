/*
 * Script to manipulate the NVS values used by bootwifi.  The script
 * dumps the contents and then can optionally:
 * 
 * * Set the values.
 * * Erase all the values.
 * 
 */
var NVS  = require("nvs.js");
var bootWiFi_ns = NVS.open("bootwifi", "readwrite");

// Retrieve any saved NVS values.
var ssid     = bootWiFi_ns.get("ssid", "string");
var password = bootWiFi_ns.get("password", "string");
var ip       = bootWiFi_ns.get("ip", "string");
var gw       = bootWiFi_ns.get("gw", "string");
var netmask  = bootWiFi_ns.get("netmask", "string");
log("Bootwifi settings");
log("ssid: " + ssid);
log("password: " + password);
log("ip: " + ip);
log("gw: " + gw);
log("netmask: " + netmask);

var setValues = false;
if (setValues) {
	bootWiFi_ns.set("ssid", "<Your SSID>", "string");
	bootWiFi_ns.set("password", "<Your Password>", "string");
	bootWiFi_ns.set("ip", "192.168.1.99", "string");
	bootWiFi_ns.set("gw", "192.168.1.1", "string");
	bootWiFi_ns.set("netmask", "255.255.255.0", "string");
}

var eraseAll = false;
if (eraseAll) {
	bootWiFi_ns.eraseAll();
}

bootWiFi_ns.close();