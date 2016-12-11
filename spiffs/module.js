FS.dump();
var fd = FS.openSync("/spiffs/test.txt", "r+");
log("FD is " + fd);
var stat = FS.fstatSync(fd);
var data = new Buffer(stat.size);
FS.readSync(fd, data, 0, data.length, null);
console.log("File content is: " + data);

FS.closeSync(fd);

log(JSON.stringify(FS.statSync("/spiffs/test.txt")));