/*
 * The Non Volatile Storage Module
 */
var nvs = {
	open: function(namespace, mode) {
		var handle = _NVS.open(namespace, mode);
		var dirty = false;
		return {
			//
			// close
			//
			close: function() {
				_NVS.close(handle);
				if (dirty) {
					log("We closed an NVS handle without a previous commit and data was dirty");
				}
				return;
			}, // close
			
			//
			// commit
			//
			commit: function() {
				_NVS.commit(handle);
				dirty = false;
				return;
			}, // commit
			
			//
			// erase
			//
			erase: function(key) {
				_NVS.erase(handle, key);
				dirty = true;
				return;
			}, // erase
			
			//
			// eraseAll
			//
			eraseAll: function() {
				_NVS.eraseAll(handle);
				dirty = true;
				return;
			}, // eraseAll
			
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
				dirty = true;
				return;
			} // set
		};
	} // open
}; // nvs
module.exports = nvs;