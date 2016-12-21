var net = require("net");
function net_test1() {
	console.log("Testing net module");
	var address = "192.168.1.105"; // httpbin.org
	var client = net.connect({address: address, port: 8001, path: "/"}, function() {
		log("We have connected!");
		client.on("data", function(data) {
			log("We have received new data over the socket!: " + data.toString());
		});
	});
}

ESP32.setLogLevel("*", "debug");
net_test1();