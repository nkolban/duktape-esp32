/*
 * SSL module.
 */

/* globals ESP32, log, module */


var moduleSSL = ESP32.getNativeFunction("ModuleSSL");
if (moduleSSL === null) {
	log("Unable to find ModuleSSL");
	module.exports = null;
	return;
}

var internalSSL = {};
moduleSSL(internalSSL);

var ssl = {
	test: internalSSL.test,
	connect: internalSSL.connect,
	createSSLSocket: internalSSL.createSSLSocket
};


module.exports = ssl;