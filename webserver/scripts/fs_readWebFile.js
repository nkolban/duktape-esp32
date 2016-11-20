var fd = FS.openSync("/web/data/text1.txt");
var myBuffer = new Buffer(100);
var readSize = FS.readSync(fd, myBuffer, 0, myBuffer.length, null);
console.log("We read:\n");
console.log(myBuffer.toString("utf8", 0, readSize));
FS.closeSync(fd);