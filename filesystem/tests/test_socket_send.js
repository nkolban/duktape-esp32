/*
 * In this sample we want to listen for incoming socket clients and when one attaches,
 * periodically send it data.  This will illustrate the technique of sending data.
 */

var net = require("net");
function createListener() {
	
	function connectionListener(sock) {
	   log("connectionListener: We have received a new connection!");
	   sock.on("data", function(data) {
	      log("connectionListener: Data received was: " + data);
	   });
	   sock.on("end", function() {
	      log("connectionListener: connection ended!");
	   });
	}
	var server = net.createServer(connectionListener);
	server.listen(8888);
}


WIFI.disconnect();
WIFI.connect({
	ssid: "sweetie",
	password: "l16wint!",
   network: {
      ip: "192.168.1.99",
      gw: "192.168.1.1",
      netmask: "255.255.255.0"
  }
}, function() {
	log("Connected! - free heap: " + ESP32.getState().heapSize);
	WIFI.setDNS(["8.8.8.8", "8.8.4.4"]);
	createListener();
});