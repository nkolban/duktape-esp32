WIFI.connect({
	ssid: "guest",
	password: "kolbanguest",
	network: {
		ip: "192.168.1.211",
		gw: "192.168.1.1",
		netmask: "255.255.255.0"
	}
}, function() {
	log("### We are now connected!");
});
log("Stash data: " + JSON.stringify(callbackStash));