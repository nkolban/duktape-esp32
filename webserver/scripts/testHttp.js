//Lets require/import the HTTP module
var http = require("module_js/http.js");

//Lets define a port we want to listen to
const PORT=80; 

//We need a function which handles requests and send response
function requestHandler(request, response){
	console.log("We have received a handleRequest!\n");
	if (request == null) {
		console.log("Odd .. the request is null");
	} else {
		console.log("Request\n");
		console.log(" - url: " + request.url + "\n");
		console.log(" - method: " + request.method + "\n");
	}
	if (response != null) {
		response.end('It Works! Path Hit: ' + request.url + "\n");
	} else {
		console.log("Odd .. we have  request but no response object!\n");
	}
}

//Create a server
var server = http.createServer(requestHandler);

//Lets start our server
server.listen(PORT, function(){
    //Callback triggered when server is successfully listening. Hurray!
    console.log("Server listening on: http://localhost:" + PORT + "\n");
});