var moduleNVS = ESP32.getNativeFunction("ModuleNVS");
if (moduleNVS === null) {
	log("Unable to find ModuleNVS");
	module.exports = null;
	return;
}

var internalNVS = {};
moduleNVS(internalNVS);
/*
 * The Non Volatile Storage Module
 */
var nvs = {
	open: function(namespace, mode) {
		var handle = internalNVS.open(namespace, mode);
		var dirty = false;
		return {
			//
			// close
			//
			close: function() {
				internalNVS.close(handle);
				if (dirty) {
					log("We closed an NVS handle without a previous commit and data was dirty");
				}
				return;
			}, // close
			
			//
			// commit
			//
			commit: function() {
				internalNVS.commit(handle);
				dirty = false;
				return;
			}, // commit
			
			//
			// erase
			//
			erase: function(key) {
				internalNVS.erase(handle, key);
				dirty = true;
				return;
			}, // erase
			
			//
			// eraseAll
			//
			eraseAll: function() {
				internalNVS.eraseAll(handle);
				dirty = true;
				return;
			}, // eraseAll
			
			//
			// get
			//
			get: function(key, type) {
				return internalNVS.get(handle, key, type);
			}, // get
			
			//
			// set
			//
			set: function(key, value, type) {
				internalNVS.set(handle, key, value, type);
				dirty = true;
				return;
			} // set
		};
	} // open
}; // nvs
module.exports = nvs;