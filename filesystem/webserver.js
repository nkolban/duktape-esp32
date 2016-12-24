/**
 * Provide a built-in WebServer used to service Duktape commands.  Primarily
 * a REST service provided.  The following are exposed:
 * 
 * POST /run - Execute the JavaScript code supplied in the POST data.
 * GET  /run/<name> - Execute the contents of the file named.
 * GET  /files - Return a JSON encoded list of files.
 * GET  /files/<name> - Return the contents of the named file.
 * POST /files/<name> - Save the content of the POST data in the named file.
 * GET  /<other> - Return the contents of the path as a regular WebServer.
 * 
 */
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
   
   //
   // request.on("data")
   //
   request.on("data", function(data) {
      log("HTTP Request on(data) passed: \"" + data + "\"");
      DUKF.logHeap("request(onData)");
      postData += data;
   }); // on("data")
   
   //
   // request.on("end")
   //
   request.on("end", function() {
   	DUKF.logHeap("request(onEnd)");
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
      // pathParts[0] = the first part of the path
      // request.method = the command.
      if (pathParts[0] == "run" && request.method == "POST") {
      	// request to run a script ...
      	log("We are about to run: " + postData);
      	try {
      		eval(postData);
      	} catch(e) {
      		//log(e.name + ": " + e.message);
      		//log("" + e.fileName + " [" + e.lineNumber + "]");
      		log(e.stack);
      	}
      	response.writeHead(200);
      } // Path begins "/run" and method is "POST"
      else if (pathParts[0] == "run" && request.method == "GET") {
      	if (pathParts.length > 1) {
      		var fileName = "/" + pathParts.splice(1).join("/");
         	DUKF.runFile(fileName);
      	}
      	response.writeHead(200);
      } 
      else if (pathParts[0] == "files") {
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
            	sendFile(DUKF.FILE_SYSTEM_ROOT + fileName, response);
      		}
      		else if (request.method == "POST") {
      			log("Writing to file " + fileName);
      			log("Data: " + postData);
      			saveFile(DUKF.FILE_SYSTEM_ROOT + fileName, postData);
      		}
      	}
      }  else {
	      try {
	      	var fileName = DUKF.FILE_SYSTEM_ROOT + request.path;
	      	FS.statSync(fileName);
	         response.writeHead(200);
	      	sendFile(fileName, response);
	      } catch(e) {
	      	log("We got an exception: " + e);
	         response.writeHead(404);
	      }
      }
      postData = null;
      response.end();
   }); // on("end")
} // requestHandler


var server = http.createServer(requestHandler);
var PORT = 8000;
server.listen(PORT);
log("WebServer listening on port " + PORT);