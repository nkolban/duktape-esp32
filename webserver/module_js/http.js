function httpServer() {
	return {
		listen: function(port, listeningCallback) {
			console.log("httpServer(): Do Listen\n");
			// Here we call mongoose to start listening for an incoming
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
	}
};

exports.createServer = function(handleRequest) {
	return httpServer();
}