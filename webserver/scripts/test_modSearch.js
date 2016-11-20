Duktape.modSearch = function(id, require, exports, module) {
	console.log("Request to load a module called " + id);
	var fd = FS.openSync("/web/"+id);
	var myBuffer = new Buffer(4000);
	var readSize = FS.readSync(fd, myBuffer, 0, myBuffer.length, null);
	FS.closeSync(fd);
	return myBuffer.toString("utf8", 0, readSize);
};