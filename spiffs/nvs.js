/*
 * The Non Volatile Storage Module
 */
var nvs = {
	open: function(namespace, mode) {
		var handle = _NVS.open(namespace, mode);
		return {
			//
			// close
			//
			close: function() {
				_NVS.close(handle);
				return;
			}, // close
			
			//
			// get
			//
			get: function(key, type) {
				return _NVS.get(handle, key, type);
			}, // get
			
			//
			// set
			//
			set: function(key, value, type) {
				_NVS.set(handle, key, value, type);
				return;
			} // set
		};
	} // open
}; // nvs
module.exports = nvs;