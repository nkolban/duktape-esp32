/*
 * SSL module.
 * 
 * We already have a Socket() class from the "net" module which can send and receive
 * data over sockets.  On top of this we want to map the ability to work with SSL based
 * encryption.
 * 
 * Imagine incoming data.
 * 
 * [Partner] (send) -----------> (recv) [ESP32-Duktape]
 * 
 * If this is coming in over an SSL transmission, then the data being passed
 * will be encrypted and is of no use to the higher layers without decryption.
 * In our underlying sockets functions, we can register an on("data") handler
 * to receive the incoming data.  In our solution we need to intercept that data and
 * pass it through the mbedtls decryption functions.  In pseudo code we need:
 * 
 * ```
 * rawSocket.on("data", function(data) {
 *    // here ... data is the encrypted data.   Now we need to do something similar to
 *    // the following.
 *    int length_read = mbedtls_ssl_read(buffer, length);
 * });
 * ```
 * 
 * However ... a call to mbedtls_ssl_read() expects to read the data from the network.  But
 * wait ... we can't do that ... we ALREADY have the data!!!  What we want is something like:
 * 
 * ```
 * dukf_ssl_read(inputData, inputLength, outputData, outputLength)
 * ```
 */

/* globals ESP32, log, module, require */

var net = require("net");

var moduleSSL = ESP32.getNativeFunction("ModuleSSL");
if (moduleSSL === null) {
	log("Unable to find ModuleSSL");
	module.exports = null;
	return;
}

var internalSSL = {};
moduleSSL(internalSSL);

var ssl = {
	//
	// debugThreshold
	//
	debugThreshold: internalSSL.debugThreshold,
	
	// Create an SSL socket
	//
	// Socket
	//
	Socket: function(options) {
		var rawSocket = new net.Socket(options);
		var dukf_ssl_context;

		var _onClose = null;
		var _onConnect = null;
		var _onData = null;
		var _onEnd = null;
		rawSocket.on("close", function() {
			if (_onClose !== null) {
				_onClose();
			}
		});
		rawSocket.on("connect", function() {
			if (_onConnect !== null) {
				_onConnect();
			}
		});
		rawSocket.on("data", function(data) {
			if (_onData !== null) {
				_onData(data);
			}
		});
		rawSocket.on("end", function(data) {
			if (_onEnd !== null) {
				_onEnd(data);
			}
		});
		return {
			on: function(eventType, callback) {
				if (eventType == "close") {
					_onClose = callback;
					return;
				}
				if (eventType == "connect") {
					_onConnect = callback;
					return;
				}
				if (eventType == "data") {
					_onData = callback;
					return;
				}
				if (eventType == "end") {
					_onEnd = callback;
					return;
				}
			},
			write: function(data) {
				// Convert the data to encrypted data and then send it.
				return rawSocket.write(data);
				//return internalSSL.write(dukf_ssl_context, data);
			},
			end: function(data) {
				return rawSocket.end(data);
			},
			//
			// connect
			//
			// Note that this is a connect on a socket  ...
			connect: function(options, callback) {
				return rawSocket.connect(options, function() {
					// rawSocket is now connected
					log("Inside SSL Connect!");
// At this point we have a socket connection to the target ... now we want to perform the
// SSL initialization.
					dukf_ssl_context = internalSSL.create_dukf_ssl_context(options.address, rawSocket.getFD());
					rawSocket.dukf_ssl_context = dukf_ssl_context;
					if (callback) {
						callback();
					}
				});
			} // connect
		}; // End of return of Socket()
	} // End of socket
};


module.exports = ssl;