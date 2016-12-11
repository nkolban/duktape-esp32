var http = require("http");
function requestHandler(request, response) {
   log("We have received a new HTTP client request!");
   request.on("data", function(data) {
      log("HTTP Request handler: " + data);
   });
   request.on("end", function() {
      log("HTTP request received:");
      log(" - method: " + request.method);
      log(" - path: " + request.path);
      log(" - headers: " + JSON.stringify(request.headers));
      response.writeHead(200);
      response.write("Hello world!");
      response.end();
   });
}
var server = http.createServer(requestHandler);
server.listen(80);

setInterval(function() {
	log("Heap: " + ESP32.getState().heapSize);
	log("Sockets: "+ JSON.stringify(_sockets));
}, 1000);