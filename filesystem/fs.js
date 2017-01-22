var moduleFS = ESP32.getNativeFunction("ModuleFS");
if (moduleFS === null) {
	log("Unable to find ModuleFS");
	module.exports = null;
	return;
}

var internalFS = {};
moduleFS(internalFS);

var fs = {
	closeSync: internalFS.closeSync,
	dump:      internalFS.dump,
	fstatSync: internalFS.fstatSync,
	openSync:  internalFS.openSync,
	readSync:  internalFS.readSync,
	spiffsDir: internalFS.spiffsDir,
	statSync:  internalFS.statSync,
	unlink:    internalFS.unlink,
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
	writeSync: internalFS.writeSync
};

module.exports = fs;