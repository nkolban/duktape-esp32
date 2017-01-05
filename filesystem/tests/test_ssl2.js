/*
 * This sample makes an HTTP request call using raw sockets over an SSL network.
 * The target is the web site https://httpbin.org/get which is an Internet resource
 * for testing HTTP protocol.
 * 
 * In the following, change the SSID and PASSWORD to be those of your own local
 * environment.
 * 
 */
var SSID="guest";
var PASSWORD="kolbanguest";

var SSL = require("ssl");


function callSSL() {
	log("Creating ssl socket!");
	var socket = new SSL.Socket();
	log("Socket created ...");
	socket.on("data", function(data){
		log("SSL socket got some data!: " + data);
	});
	socket.on("end", function() {
		log("SSL connection ended");
	});
	socket.connect({
		address: "httpbin.org",
		port: 443
	}, function() {
		log("Socket connected!");
		socket.write(new Buffer("GET /get HTTP/1.1\r\nHost: www.example.org\r\nConnection: close\r\n\r\n"));
	});
	log("End");
} // callSSL

// Note: We assume here that we are NOT network connected and hence connect first.  If we
// know that we are network connected to start, we can invoke "callSSL()" immediately.

//SSL.debugThreshold(4); // Debug level for SSL logging.
WIFI.disconnect();
WIFI.connect({
	ssid: SSID,
	password: PASSWORD,
   network: {
      ip: "192.168.1.211",
      gw: "192.168.1.1",
      netmask: "255.255.255.0"
  }
}, function(err) {
	log("Error was: " + err);
	if (err !== null) {
		return;
	}
	log("Connected! - free heap: " + ESP32.getState().heapSize);
	WIFI.setDNS(["8.8.8.8", "8.8.4.4"]);

	callSSL();
});