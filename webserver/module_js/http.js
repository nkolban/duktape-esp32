// This function is called when we receive an incoming request from the HTTP
// server.
function httpServerRequestReceivedCallback(url, method) {
	//console.log("httpServerRequestReceivedCallback: We received a request!\n");
	//console.log("- url: " + url + "\n");
	//console.log("- method: " + method + "\n");
	//console.log(" - this: " + this);
	var request = {
		url: url,
		method: method
	};
	var response = {
		statusCode: 200,
		body: "",
		writeHead: function(statusCode, headers) {
			this.statusCode = statusCode;
		},
		write: function(data) {
			this.body = this.body + data;
		},
		// When we get an end notification, this is our clue to invoke mongoose
		// to actually send the response back to the client that is waiting for the response.
		// We build the parameters and then send them via the native call.
		end: function(data) {
			this.body = this.body + data;
			var serverResponseMongoose = ESP32.getNativeFunction("serverResponseMongoose");
			serverResponseMongoose(this.statusCode, null /*headers*/, this.body);
		}
	}; // End of response
	_httpServerRequestReceivedCallback.handleRequest(request, response);
} // requestReceivedCallback

function httpServer() {
	return {
		listen: function(port, listeningCallback) {
			console.log("httpServer(): Do Listen\n");
			// Here we call mongoose to start listening for an incoming
			var mongooseStart = ESP32.getNativeFunction("startMongoose");
			mongooseStart();
			
			// HTTP request.
			if (listeningCallback != null) {
				listeningCallback();
			}
		}, // listen
		
		close: function(closeCallback) {
			console.log("httpServer(): Do close\n");
			if (closeCallback) {
				closeCallback();
			}
		} // close
	};
} // httpServer

/**
 * The handleRequest is a function with the following signature:
 * handleRequest(request, response)
 * Request maps to the class called "http.IncomingMessage"
 * 
 */
exports.createServer = function(handleRequest) {
	httpServerRequestReceivedCallback.handleRequest = handleRequest;
	// Save the requestReceivedCallback as a global.
	_httpServerRequestReceivedCallback = httpServerRequestReceivedCallback;

	return httpServer();
};