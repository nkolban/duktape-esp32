var http = require("http");
function requestHandler(request, response) {
   log("We have received a new HTTP client request!");
}
var server = http.createServer(requestHandler);
server.listen(80);