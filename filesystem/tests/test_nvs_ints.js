var NVS = require("nvs");
var nvsNamespace = NVS.open("neil", "readwrite");
try {
    var val = nvsNamespace.get("i1", "int");
    log("Value of i1 is " + val);
} catch(e) {
    log(e);
}


nvsNamespace.set("i1", 9876, "int");
try {
    var val = nvsNamespace.get("i2", "int");
    log("Value of i2 is " + val);
}
catch(e) {
    log(e);
}
nvsNamespace.close();