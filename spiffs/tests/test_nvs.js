var NVS = require("nvs");
var nvsNamespace = NVS.open("neil", "readwrite");
var val = nvsNamespace.get("a1", "string");
log("Value of a1 is " + val);
nvsNamespace.set("a1", "Kolban", "string");
var val = nvsNamespace.get("a2", "string");
log("Value of a2 is " + val);
nvsNamespace.close();