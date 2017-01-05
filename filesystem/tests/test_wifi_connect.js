var net = require("net");
WIFI.disconnect();
WIFI.connect({
	ssid: "guest",
	password: "kolbanguest",
   network: {
      ip: "192.168.1.211",
      gw: "192.168.1.1",
      netmask: "255.255.255.0"
  }
}, function() {
	log("Connected! - free heap: " + ESP32.getState().heapSize);
	WIFI.setDNS(["8.8.8.8", "8.8.4.4"]);
});