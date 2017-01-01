/*
 * Serial module.
 */

/* globals ESP32, log, module, LOGE, Buffer, cancelInterval */

var POLL_INTERVAL = 100;
var BUFFER_SIZE   = 128;

var moduleSerial = ESP32.getNativeFunction("ModuleSerial");
if (moduleSerial === null) {
	log("Unable to find ModuleSerial");
	module.exports = null;
	return;
}

var internalSerial = {};
//Populate the internalSerial object with the methods
//that are implemented in C.  These will include:
//* configure()
//* read()
//* write()
//
moduleSerial(internalSerial);


// Create a new instance of a SerialPort class that is associated
// with the port.
function serial(port) {
	// Validate that the port number is in range.
	if (port < 0 || port > 2) {
		LOGE("Invalid port numbed");
		return null;
	}
	var intervalTimerId = -1;
	
	var dataCallback = null; // The callback to be invoked when new data arrives.
	var buffer = new Buffer(BUFFER_SIZE);
	var retObj = {
		//
		// configure
		//
		configure: function(options) {
			return internalSerial.configure(port, options);
		},
		
		//
		// on
		//
		on: function(eventType, callback) {
			if (eventType == "data") {
				dataCallback = callback;
				if (intervalTimerId != -1) {
					cancelInterval(intervalTimerId);
					intervalTimerId = -1;
				}
// The user can cancel further events by supplying a callback function of null and
// the interval timer will stop.
				if (callback == null) {
					return;
				}
				
				intervalTimerId = setInterval(function() {
					var sizeRead = internalSerial.read(port, buffer);
					if (sizeRead === 0) {
						return;
					}
					//log("We read " + sizeRead + " bytes from port " + port);
					callback(buffer.slice(0, sizeRead));
				}, POLL_INTERVAL);
			} // on("data")
		}, // on
		
		//
		// read
		//
		read: function(buffer) {
			if (dataCallback != null) {
				log("Can't do explicit serial read while callback in effect");
				return 0;
			}
			return internalSerial.read(port, buffer);	
		},
		
		//
		// write
		//
		write: function(data) {
			return internalSerial.write(port, data);	
		}
	};
	return retObj;
} // serial

module.exports = serial;