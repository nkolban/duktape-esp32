//DUKF.debug();
var PORT = 8002;
var ws = require("ws.js");
log("Being a WebSocket server on port " + PORT);
var websocketServer = ws.Server(PORT);
websocketServer.on("connection", function(wsConnection) {
	log("We have received a new WebSocket connection");
	wsConnection.on("message", function(data) {
		log("We have received an incoming message: " + data);
		wsConnection.send("Hello back");
		wsConnection.close();
	});
});