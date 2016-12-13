var http = require("http");
function requestHandler(request, response) {
   log("WebServer: We have received a new HTTP client request!");
   var postData = "";
   request.on("data", function(data) {
      log("HTTP Request on(data) passed: " + data);
      postData += data;
   });
   request.on("end", function() {
      log("HTTP Request on(end):");
      log(" - method: " + request.method);
      log(" - path: " + request.path);
      log(" - headers: " + JSON.stringify(request.headers));
      response.writeHead(200);
      
      if (request.path == "/run") {
      	// request to run a script ...
      	log("We are about to run: " + postData);
      	try {
      		eval(postData);
      	} catch(e) {
      		log("Oh my!! The code we read over the net threw an exception! - " + e);
      	}
      } else {
	      try {
		      var fd = FS.openSync("/spiffs" + request.path, "r");
		      var buffer = new Buffer(128);
		      // Do something else ...
		      while(1) {
		      	var sizeRead = FS.readSync(fd, buffer, 0, buffer.length, null);
		      	log("Size read: " + sizeRead);
		      	if (sizeRead <= 0) {
		      		break;
		      	}
		      	response.write(buffer.toString("utf8", 0, sizeRead));
		      }
		      FS.closeSync(fd);
	      } catch(e) {
	      	log("We got an exception: " + e);
	      }
      }
      response.end();
   });
}

var server = http.createServer(requestHandler);
server.listen(80);

setInterval(function() {
	var heap = ESP32.getState().heapSize;
	if (heap < 20000) {
		log("Heap: " + ESP32.getState().heapSize);
		log("Sockets: "+ JSON.stringify(_sockets));
	}
}, 1000);