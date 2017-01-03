var SSL = require("ssl");
var net = require("net");
WIFI.disconnect();
WIFI.connect({
	ssid: "sweetie",
	password: "password",
   network: {
      ip: "192.168.1.211",
      gw: "192.168.1.1",
      netmask: "255.255.255.0"
  }
}, function() {
	log("Connected!");
	log("DNS: " + JSON.stringify(WIFI.getDNS()));
	WIFI.setDNS(["8.8.8.8", "8.8.4.4"]);
	log("DNS: " + JSON.stringify(WIFI.getDNS()));
	var ip = net.getByName("httpbin.org");
	log("IP: " + ip);

	log("Creating ssl socket!");
	var socket = SSL.createSSLSocket();
	SSL.connect(socket, "httpbin.org", 443);
});