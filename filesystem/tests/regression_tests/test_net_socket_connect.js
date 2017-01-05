// Test that we can form a connection
log("On Linux, run \"nc -l -p 9999\" in a shell");
log("We will next attempt to connect to it and write some data");
var HOST = "192.168.1.105";
var PORT = 9999;
var net = require("net");
var socket = net.connect({
	address: HOST,
	port: PORT
}, function() {
	if (typeof socket !== "object") {
		throw new Error("socket was not an object! : " + socket);
	}
	log("We are now connected ...");
	log("Writing some data ...");
	socket.write("Hello world");
	log("Closing the connection ...");
	socket.end();
});