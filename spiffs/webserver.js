/* globals require, FS, Buffer, log, ESP32, _sockets*/

var http = require("http.js");
var URL = require("url.js");

/**
 * Read a file from the SPIFFS file system and send it to the output stream.
 * @param fileName The name of the file to read.
 * @param response The target where we are writing the file.
 * @returns 0 on ok, non 0 on an error.
 */
function sendFile(fileName, response) {
	var fd = FS.openSync(fileName, "r");
   var buffer = new Buffer(128);
   while(1) {
   	var sizeRead = FS.readSync(fd, buffer, 0, buffer.length, null);
   	log("Size read: " + sizeRead);
   	if (sizeRead <= 0) {
   		break;
   	}
   	response.write(buffer.toString("utf8", 0, sizeRead));
   }
   FS.closeSync(fd);
   return 0;
} // sendFile


/**
 * Save the data as a local file.
 * @param fileName The file name to create.
 * @param data The data to write.
 * @returns N/A
 */
function saveFile(fileName, data) {
	var fd = FS.openSync(fileName, "w");
	FS.writeSync(fd, data);
	FS.closeSync(fd);
} // saveFile


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

      log("URL: " + JSON.stringify(URL.parse(request.path)));
      var pathParts = request.path.split("/");
      if (pathParts.length < 2) {
         response.end();
         return;
      }
      pathParts = pathParts.splice(1); // Remove the first element which will be "/"
      if (pathParts[0] == "run") {
      	// request to run a script ...
      	log("We are about to run: " + postData);
      	try {
      		eval(postData);
      	} catch(e) {
      		log("Oh my!! The code we read over the net threw an exception! - " + e);
      	}
      	response.writeHead(200);
      } else if (pathParts[0] == "files") {
         response.writeHead(200);
      	// Process files here ...
      	// if GET /files  -- then pathParts.length == 1
      	if (pathParts.length == 1 && request.method == "GET") {
      		var filesArray = FS.spiffsDir();
         	response.write(JSON.stringify(filesArray));
      	} else { // More parts than just /files
      		var fileName = "/" + pathParts.splice(1).join("/");
      		if (request.method == "GET") {
            	log("Load the file called " + fileName);
            	sendFile("/spiffs" + fileName, response);
      		}
      		else if (request.method == "POST") {
      			log("Writing to file " + fileName);
      			log("Data: " + postData);
      			saveFile("/spiffs" + fileName, postData);
      		}
      	}
      }  else {
	      try {
	      	var fileName = "/spiffs" + request.path;
	      	FS.statSync(fileName);
	         response.writeHead(200);
	      	sendFile(fileName, response);
	      } catch(e) {
	      	log("We got an exception: " + e);
	         response.writeHead(404);
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