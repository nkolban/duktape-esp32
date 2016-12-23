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
});
webSocketServer.listen(PORT);
log("Being a WebSocket server on port " + PORT);