log(JSON.stringify(Duktape.env));
try {
	ESP32.debug();
var fd = FS.openSync("/spiffs/web/ideXXX.html", "r");
var buffer = new Buffer(128);
// Do something else ...
while(1) {
	var sizeRead = FS.readSync(fd, buffer, 0, buffer.length, null);
	log("Size read: " + sizeRead);
	if (sizeRead <= 0) {
		break;
	}
}
FS.closeSync(fd);
} catch(e) {
	log("We got an exception: " + e);
}