var http = require("http");
function requestHandler(request, response) {
   log("We have received a new HTTP client request!");
   request.on("data", function(data) {
   	log("HTTP Request handler: " + data);
   });
   request.on("end", function() {
   	log("HTTP request received:");
   	log(" - method: " + request.method);
   	log(" - headers: " + JSON.stringify(request.headers));
   });
}
var server = http.createServer(requestHandler);
server.listen(80);