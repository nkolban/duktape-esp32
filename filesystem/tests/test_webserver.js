var HTTP = require("http");
var URL = require("url");
var FS = require("fs");


var WEBSERVER_PORT = 80;
//var WEBSOCKET_PORT = 8002;

var server = HTTP.createServer(requestHandler);
server.listen(WEBSERVER_PORT);
log("WebServer listening on port " + WEBSERVER_PORT);

function requestHandler(request, response) {
   log("WebServer: We have received a new HTTP client request!");
   var postData = "";
   request.on("data", function(data) {
      log("HTTP Request on(data) passed: " + data);
      postData += data;
   }); // on("data")
   request.on("end", function() {
   	switch(request.path) {
   	case "/":
   		break;
   	default:
   		response.writeHead(404);
   	}
      response.end();
   }); // on("end")
} // requestHandler