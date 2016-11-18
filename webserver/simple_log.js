console.log("We were loaded!\n");
Duktape.modSearch = function(id, require, exports, module) {
	console.log("Request to load module with name: " + id + "\n");
}
var x = require("MyModule");
