var ws = require("ws.js");
var websocketServer = ws.Server(81);
websocketServer.on("connection", function(wsConnection) {
	log("We have received a new WebSocket connection");
	wsConnection.on("message", function(data) {
		log("We have received an incoming message: " + data);
		wsConnection.send("Hello back");
		wsConnection.close();
	});
});