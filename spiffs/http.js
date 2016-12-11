/**
 * Implement the HTTP classes.
 */
/* globals require, log, module */
/* exported http */

var net = require("net");
var Stream = require("stream");
var HTTPParser = require("httpparser");

/**
 * Given a line of text that is expected to be "\r\n" delimited, return an object that
 * contains the first line and the remainder.
 * @param data The input text to parse.
 * @returns An object that contains
 * {
 *    line: <The line of text up to the first \r\n>
 *    remainder: The remainder of the data after the first \r\n or the whole
 *       data if there is no first \r\n.
 * }
 */
function getLine(data) {
	var i = data.indexOf("\r\n");
	if (i === -1) { // If we didn't find a terminator, then no lines and remainder is all.
		return {
			line: null,
			remainder: data
		};
	}
	return {
		line: data.substr(0, i),
		remainder: data.substr(i+2)
	};
} // getLine


/* Getline tests ...
var text = "1234567890\r\nabcdefghij\r\n";
log(JSON.stringify(getLine(text)));
text = "1234567890\r\n";
log(JSON.stringify(getLine(text)));
text = "1234567890";
log(JSON.stringify(getLine(text)));
*/

/**
 * The HTTP Module.
 * request - 
 */
var http = {
	// Send an HTTP request.  The options object contains the details of the request
	// to be sent.
	// {
	//    address: <IP Address of target of request>
	//    port: <port number of target> [optional; default=80]
	//    path: <Path within the url> [optional; default="/"]
	// }
	//
	// The high level algorithm is that we create a new socket and then issue a connect
	// request upon it.  When we have detected that the connect has succeeded, we then
	// send an HTTP protocol request down the socket.  We register that we are
	// interested in receiving the response and parse the response assuming it is an
	// HTTP message.
	request: function(options, callback) {
		if (options.address === undefined) {
			log("http.request: No address set");
			return;
		}
		
		if (options.path === undefined) {
			options.path = "/";
		}


		if (options.port === undefined) {
			options.port = 80;
		}
		
		var sock = new net.Socket();
		var clientRequest = {
			_sock: sock	
		};
		callback(sock);
		var message = "";
		// Send a connect request and register the function to be invoked when the connect
		// succeeds.  That function is responsible for sending the HTTP request to the partner.
		sock.connect(options, function() {
			// We are now connected ... send the HTTP message
			// Build the message to send.
			var requestMessage = "GET " + options.path + " HTTP/1.0\r\n" + 
			"User-Agent: ESP32-Duktape\r\n" +
			"\r\n";
			sock.write(requestMessage); // Send the message to the HTTP server.
		}); // sock.connect connectionListener ...
		
		sock.on("data", function(data) {
			message += data.toString();
		});
		
		sock.on("end", function() {
			var STATE = {
				START: 1,
				HEADERS: 2,
				BODY: 3
			};
			log("HTTP session over ... data is: " + message);
			// We now have the WHOLE HTTP response message ... so it is time to parse the data
			var state = STATE.START;
			var line = getLine(message);
			var headers = {};
			while (line.line !== null) {
				if (state == STATE.START) {
// A header line is of the form <protocols>' '<code>' '<message>
//                                   0          1         2					
					var splitData = line.line.split(" ");
					var httpStatus = splitData[1];
					log("httpStatus = " + httpStatus);
					// We found the start line ...
					state = STATE.HEADERS;
				} // End of in STATE.START
				else if (state == STATE.HEADERS) {
// Between the headers and the body of an HTTP response is an empty line that marks the end of
// the HTTP headers and the start of the body.  Thus we can find a line that is empty and, if it
// is found, we switch to STATE.BODY.  Otherwise, we found a header line of the format:
// <name>':_'<value>
					if (line.line.length === 0) {
						log("End of headers\n" + JSON.stringify(headers));
						state = STATE.BODY;
					} else {
						// we found a header
						var i = line.line.indexOf(":");
						var name = line.line.substr(0, i);
						var value = line.line.substr(i+2);
						headers[name] = value;
					}
				} // End of in STATE.HEADERS
				
				log("New line: " + line.line);
				line = getLine(line.remainder);
			} // End of we have processed all the lines.
			log("Final remainder: " + line.remainder);
			log("Remainder length: " + line.remainder.length);
		}); // on("end");
		return clientRequest;
	}, // request
	
	//
	// createServer
	//
	// Create a socket that is not yet bound to a port.  The caller will later bind the server
	// to a port and start it listening.  We register an internal connectionListener that is
	// invoked when ever a new HTTP client connects to our listening port.  That connection
	// listener will handle the incoming request and invoke the requestHandler supplied by the
	// user.
	createServer: function(requestHandler) {
		// Internal connection listener that will be called when a new client connection
		// has been received.

		var connectionListener = function(sock) {
			var httpRequestStream = new Stream();
			requestHandler(httpRequestStream.reader, null);
			var parserStreamWriter = new HTTPParser(HTTPParser.REQUEST, function(parserStreamReader) {
				parserStreamReader.on("data", function(data) {
					httpRequestStream.reader.method = parserStreamReader.method;
					httpRequestStream.reader.path = parserStreamReader.path;
					httpRequestStream.reader.headers = parserStreamReader.headers;
					httpRequestStream.writer.write(data);
				});
				parserStreamReader.on("end", function() {
					httpRequestStream.reader.method = parserStreamReader.method;
					httpRequestStream.reader.path = parserStreamReader.path;
					httpRequestStream.reader.headers = parserStreamReader.headers;
					httpRequestStream.writer.end();
				});
			});
			sock.on("data", function(data) {
				parserStreamWriter.write(data);
			});
			sock.on("end", function() {
				parserStreamWriter.end();
			});
		};
		var socketServer = net.createServer(connectionListener);
		return socketServer; // Return the socket server which has a listen() method to start it listening.
	} // createServer
}; // http

module.exports = http; // Export the http function
