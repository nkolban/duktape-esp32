var moduleSPI = ESP32.getNativeFunction("ModuleSPI");
if (moduleSPI === null) {
	log("Unable to find ModuleSPI");
	module.exports = null;
	return;
}

var internalSPI = {};
moduleSPI(internalSPI);

var ret = {
	//
	// initialize
	//
	// options includes:
	// {
	//    host - Optional - Default to HSPI_HOST
	//    mosi -
	//    miso -
	//    clk -
	// }
	initialize: function(options) {
		internalSPI.bus_initialize(options);
	}, // initialize
	
	free: function(host) {
		internalSPI.bus_free(host);
	}, // free
	
	//
	// addDevice
	//
	// {
   //    host: <optional host. Default: HSPI_HOST>
   //    mode: <optional mode.  Default: 0>
   //    clock_speed: <optional clock speed in Hz.  Default: 10000 (10KHz)>
   //    cs: <pin to use for CS.  Default: -1 (not used)>
	// }
	addDevice: function(options) {
		var handle = internalSPI.add_device(options);
		return {
			//
			// remove
			//
			remove: function() {
				internalSPI.remove_device(handle);
				handle = null;
			}, // remove
			
			//
			// transmit
			//
			transmit: function(data) {
				internalSPI.transmit(handle, data);
			}, // transmit
		}
	} // addDevice
}; // ret

module.exports = ret;