var PORT = 8002;
var ws = require("ws.js");
var webSocketServer = ws.Server();

webSocketServer.on("connection", function(wsConnection) {
	log("We have received a new WebSocket connection.  The path is \"" + wsConnection.path + "\"");
	wsConnection.on("message", function(data) {
		log("We have received an incoming message: " + data);
		wsConnection.send("Hello back");
		wsConnection.close();
	});
	wsConnection.on("close", function() {
		log("Web Socket connection closed, ending handler!");
		console.handler = null;
	});
	// Register a console.log() handler that will send the logged message to
	// the WebSocket.
	console.handler = function(message) {
		log("Sending to WS");
		wsConnection.send(message);
	}
});
webSocketServer.listen(PORT);
log("Being a WebSocket server on port " + PORT);