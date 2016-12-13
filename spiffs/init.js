/* globals Duktape, log, ESP32 */
/* exported _sockets */

if (!String.prototype.endsWith) {
  String.prototype.endsWith = function(searchString, position) {
      var subjectString = this.toString();
      if (typeof position !== 'number' || !isFinite(position) || Math.floor(position) !== position || position > subjectString.length) {
        position = subjectString.length;
      }
      position -= searchString.length;
      var lastIndex = subjectString.lastIndexOf(searchString, position);
      return lastIndex !== -1 && lastIndex === position;
  };
}

if (!String.prototype.startsWith) {
   String.prototype.startsWith = function(searchString, position){
     position = position || 0;
     return this.substr(position, searchString.length) === searchString;
 };
}

Duktape.modSearch = function(id, require, exports, module) {
	log("Module: Loading \"" + id + "\"");
	var name = id;
	if (!id.endsWith(".js")) {
		name += ".js";
	}
	return ESP32.loadFileESPFS(name);
};

ESP32.setLogLevel("*", "debug");

var _sockets= {};

require("loop");