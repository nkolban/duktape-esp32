var HTTP = require("http");
var FS   = require("fs");
var URL  = require("url");
var NP   = require("neopixels");
var PIN = 21;
var PIXEL_COUNT=7;

var neop = new NP(PIN, PIXEL_COUNT);

var WEBSERVER_PORT = 80;

var server = HTTP.createServer(requestHandler);
server.listen(WEBSERVER_PORT);

log("WebServer listening on port " + WEBSERVER_PORT);


function requestHandler(request, response) {
	log("Heap: "+ ESP32.getState().heapSize);
   request.on("end", function() {
   	switch(request.path) {
   	case "/setColor":
   		var values = URL.queryParse(request.query);
   		neop.clear();
   		var i;
   		for (i=0; i<PIXEL_COUNT; i++) {
   			neop.setPixel(i, values.color);
   		}
   		neop.show();
			response.writeHead(200);
   		break;
   	default:
   		var data = FS.loadFile(request.path);
   		if (data === null) {
   			response.writeHead(404);
   		} else {
   			response.writeHead(200);
   			response.write(data);
   		}
   	}
      response.end();
   }); // on("end")
} // requestHandler