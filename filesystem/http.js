/**
 * Implement the HTTP classes.
 */
/* globals require, log, module */
/* exported http */

var net = require("net.js");
var Stream = require("stream.js");
var HTTPParser = require("httpparser.js");


/**
 * The HTTP Module.
 * request - 
 */
var http = {
	//
	// request
	//
	//
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
	// send an HTTP protocol request through the socket connection.  We register that we are
	// interested in receiving the response and parse the response assuming it is an
	// HTTP message.
	request: function(options, httpRequestCallback) {
		var path = options.path;
		var port = options.port;
		var address = options.address;
		var method = options.method;
		
		// validate inputs and set defaults for unset properties.
		if (address === undefined) {
			log("http.request: No address set");
			return;
		}
		
		if (method === undefined) {
			method = "GET";
		}
		
		if (method !== "GET" && method !== "POST") {
			log("http.request: Unknown method: " + method);
			return;
		}
		
		if (path === undefined) {
			path = "/";
		}

		if (port === undefined) {
			port = 80;
		}
		

		
		var sock = new net.Socket();
		var clientRequest = {
			_sock: sock	
		};

		var httpClientResponseStream = new Stream();
		
		if (httpRequestCallback !== null && httpRequestCallback !== undefined) {
			httpRequestCallback(httpClientResponseStream.reader);
		}
		
		
		var parserStreamWriter = new HTTPParser(HTTPParser.RESPONSE, function(parserStreamReader){
			parserStreamReader.on("data", function(data) {
				httpClientResponseStream.reader.httpStatus = parserStreamReader.httpStatus;
				httpClientResponseStream.reader.headers = parserStreamReader.headers;
				httpClientResponseStream.writer.write(data);
			});
			parserStreamReader.on("end", function(){
				httpClientResponseStream.reader.httpStatus = parserStreamReader.httpStatus;
				httpClientResponseStream.reader.headers = parserStreamReader.headers;
				httpClientResponseStream.writer.end();
			});
		});


		// Send a connect request and register the function to be invoked when the connect
		// succeeds.  That registered function is responsible for sending the HTTP request to the partner.
		sock.connect({address: address, path: path, port: port}, function() {
			// We are now connected ... send the HTTP message
			// Build the message to send.
			var requestMessage = "GET " + path + " HTTP/1.0\r\n" + 
			"User-Agent: ESP32-Duktape\r\n" +
			"\r\n";
			sock.write(requestMessage); // Send the message to the HTTP server.
		}); // sock.connect connectionListener ...
		
		sock.on("data", function(data) {
			parserStreamWriter.write(data);
		});
		
		sock.on("end", function() {
			parserStreamWriter.end();
		});
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
	
// How we process an incoming HTTP request
//
// We create a connectionListener function that is called with a socket parameter when
// a new connection is formed.  This is the connection from the new HTTP client.
// We now form two streams an httpRequestStream and an httpResponseStream.
// Data received FROM the partner is written INTO the httpRequestStream.
// Data to be send TO the partner is written INTO the httpResponseStream.  We thus
// create the notion of request (data from the partner) and response (data to the partner).
//
// Next we create an HTTPParser object.  This is responsible for parsing the data
// that comes from the request.
	
	createServer: function(requestHandler) {
		// Internal connection listener that will be called when a new client connection
		// has been received.

		var connectionListener = function(sock) {
			var httpRequestStream = new Stream(); // Data FROM the partner
			var httpResponseStream = new Stream(); // Data TO the partner
			httpResponseStream.writer.writeHead = function(statusCode, param1, param2) {
				var statusMessage;
				var headers;
				if (typeof param1 == "string") {
					statusMessage = param1;
					headers = param2;
				} else {
					switch(statusCode) {	
						case 101: {
							statusMessage = "Switching Protocols";
							break;
						}
						case 200: {
							statusMessage = "OK";
							break;
						}
						case 400: {
							statusMessage = "Bad Request";
							break;
						}
						case 401: {
							statusMessage = "Unauthorized";
							break;
						}
						case 403: {
							statusMessage = "Forbidden";
							break;
						}
						case 404: {
							statusMessage = "Not Found";
							break;
						}
						default: {
							statusMessage = "Unknown";
							break;
						}
					}

					headers = param1;
				}
				sock.write("HTTP/1.1 " + statusCode + " " + statusMessage + "\r\n");
				if (headers !== undefined) {
					for (var name in headers) {
						if (headers.hasOwnProperty(name)) {
							sock.write(name + ": " + headers[name] + "\r\n");
						}
					}
				}
				sock.write("\r\n");
			};
			httpRequestStream.reader.headers = {};
			
			// request.getHeader(name) - Obtain the value of a named header.
			httpRequestStream.reader.getHeader = function(name) {
				if (httpRequestStream.reader.headers.hasOwnProperty(name)) {
					return httpRequestStream.reader.headers[name];
				}
				return null;
			}; // getHeader
			httpRequestStream.reader.getSocket = function() {
				return sock;
			};
			
			requestHandler(httpRequestStream.reader, httpResponseStream.writer);
			httpResponseStream.reader.on("data", function(data) {
				sock.write(data);
			});
			httpResponseStream.reader.on("end", function() {
				sock.end();
			});
			
			var parserStreamWriter = new HTTPParser("request", function(parserStreamReader) {
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
				//log("Received data from socket, sending to HTTP Parser");
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
