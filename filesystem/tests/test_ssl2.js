var SSL = require("ssl");
//SSL.debugThreshold(4);
var net = require("net");
WIFI.disconnect();
WIFI.connect({
	ssid: "sweetie",
	password: "l16wint!",
   network: {
      ip: "192.168.1.211",
      gw: "192.168.1.1",
      netmask: "255.255.255.0"
  }
}, function() {
	log("Connected! - free heap: " + ESP32.getState().heapSize);
	WIFI.setDNS(["8.8.8.8", "8.8.4.4"]);

	log("Creating ssl socket!");
	var socket = new SSL.Socket();
	log("Socket created ...");
	socket.connect({
		address: "httpbin.org",
		port: 443
	}, function() {
		log("Socket connected!");
		socket.write("GET / HTTP/1.1\r\n\r\n");
	});
	log("End");
});