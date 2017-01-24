var moduleFS = ESP32.getNativeFunction("ModuleFS");
if (moduleFS === null) {
	log("Unable to find ModuleFS");
	module.exports = null;
	return;
}

var internalFS = {};
moduleFS(internalFS);

var fs = {
	//
	// createWithContent
	//
	// Create a file on the file system with the specified path name
	// and the supplied content.
	createWithContent: function(path, content) {
		// open the file
		var fd = this.openSync(path, "w");
		// write the content
		this.writeSync(fd, content);
		// close the file
		this.closeSync(fd);
	}, // createWithContent
	
	closeSync: internalFS.closeSync,
	dump:      internalFS.dump,
	fstatSync: internalFS.fstatSync,
	
	//
	// loadFile
	//
	// Load the named file from the posix file system.
	// Returns a new Buffer that contains the data read.
	
	loadFile: function(path) {
		try {
			var fd = this.openSync(path, "r");
			var size = this.fstatSync(fd).size;
			var data = new Buffer(size);
			var sizeRead = this.readSync(fd, data, 0, data.length, null);
			if (sizeRead != size) {
				log("fs.loadFile: Short read ... expected " + size + " but only read " + sizeRead);
			}
			this.closeSync(fd);
			return data;
		} catch(e) {
			log("Exception: " + e.state);
			return null;
		}
	}, // loadFile
	
	openSync:  internalFS.openSync,
	readSync:  internalFS.readSync,
	spiffsDir: internalFS.spiffsDir,
	statSync:  internalFS.statSync,
	unlink:    internalFS.unlink,
	writeSync: internalFS.writeSync
};

module.exports = fs;