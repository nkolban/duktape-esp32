var fd = FS.openSync("/spiffs/a", "r");
console.log("fd = " + fd);
var buf = new Buffer(500);
var i = FS.readSync(fd, buf, 0, buf.length, 0);
console.log("We read " + i + " bytes: \"" + buf.toString("ascii", 0, i) + "\"");
FS.writeSync(fd, "Neil Was Here!");
FS.closeSync(fd);

fd = FS.openSync("/spiffs/test.txt", "w");
FS.writeSync(fd, "Neil Was Here!");
FS.closeSync(fd);
FS.dump();