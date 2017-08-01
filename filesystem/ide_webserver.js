/**
 * Provide a built-in WebServer used to service Duktape commands.  Primarily
 * a REST service provider.  The following are exposed:
 * 
 * POST /run - Execute the JavaScript code supplied in the POST data.
 * GET  /run/<name> - Execute the contents of the file named.
 * GET  /files - Return a JSON encoded list of files.
 * GET  /files/<name> - Return the contents of the named file.
 * POST /files/<name> - Save the content of the POST data in the named file.
 * GET  /<other> - Return the contents of the path as a regular WebServer.
 * 
 */
/* globals require, FS, Buffer, log, DUKF, module*/

var http = require("http");
var ws = require("ws");
var FS = require("fs");


/**
 * Read a file from the SPIFFS file system and send it to the output stream.
 * @param fileName The name of the file to read.
 * @param response The target where we are writing the file.
 * @returns 0 on ok, non 0 on an error.
 */
function streamFile(fileName, response) {
	//log("FS.openSync1");
	var fd = FS.openSync(fileName, "r");
	//log("FS.openSync2");
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
} // streamFile


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
   log("IDE_WebServer: We have received a new HTTP client request!");
   DUKF.logHeap("requestHandler");
   var postData = "";
   
   request.on("data", function(data) {
      log("HTTP Request on(data) passed: " + data);
      postData += data;
   }); // on("data")
   
   request.on("end", function() {
   	function sendFile(fileToSend) {
	      try {
	      	fileName = DUKF.FILE_SYSTEM_ROOT + fileToSend;
	      	log("File to read: " + fileName);
	      	FS.statSync(fileName); // Will throw an error if file is not present
	      	log("Loading file: " + fileName);
	         response.writeHead(200);
	         streamFile(fileName, response);
	      } catch(e) {
	      	log("(A1) We got an exception: " + e);
	         response.writeHead(404);
	      }
   	} // sendFile
      
   	var fileName;
      log("HTTP Request on(end):");
      log(" - method: " + request.method);
      log(" - path: " + request.path);
      log(" - headers: " + JSON.stringify(request.headers));
      DUKF.logHeap("ide_webserver: request.on(end)");

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
      		fileName = "/" + pathParts.splice(1).join("/");
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
      		fileName = "/" + pathParts.splice(1).join("/");
      		if (request.method == "GET") {
            	log("Load the file called " + fileName);
            	streamFile(DUKF.FILE_SYSTEM_ROOT + fileName, response);
      		}
      		else if (request.method == "POST") {
      			log("Writing to file " + fileName);
      			log("Data: " + postData);
      			saveFile(DUKF.FILE_SYSTEM_ROOT + fileName, postData);
      		}
      	}
      } else {
      	sendFile(request.path);
      }
      postData = null;
      response.end();
   }); // on("end")
} // requestHandler


/**
 * Start being an IDE
 * @returns N/A
 */
function startIde() {

	var WEBSERVER_PORT = 8000;
	var WEBSOCKET_PORT = 8002;
	
	var server = http.createServer(requestHandler);
	server.listen(WEBSERVER_PORT);
	log("IDE_WebServer listening on port " + WEBSERVER_PORT);


   if (ws !== null) {
		var webSocketServer = ws.Server();
	
		webSocketServer.on("connection", function(wsConnection) {
			log("We have received a new WebSocket connection.  The path is \"" + wsConnection.path + "\"");
			wsConnection.on("message", function(data) {
				log("We have received an incoming message: " + data);
				wsConnection.close();
			});
			
			wsConnection.on("close", function() {
				log("Web Socket connection closed, ending handler!");
				console.handler = null;
			});
			
			// Register a console.log() handler that will send the logged message to
			// the WebSocket.
			console.handler = function(message) {
				log("Sending to WS");
				wsConnection.send(message);
			};
		});
		webSocketServer.listen(WEBSOCKET_PORT);
		log("Being a WebSocket server on port " + WEBSOCKET_PORT);
   }
} // startIde

module.exports = startIde;