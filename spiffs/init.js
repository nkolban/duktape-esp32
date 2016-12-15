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

var _timers = {
	id: 1,
	timerEntries: [],
	setTimer: function(callback, interval, isInterval) {
		var id = _timers.id;
		_timers.timerEntries.push({
			id: id,
			callback: callback,
			fire: new Date().getTime() + interval,
			interval: isInterval?interval:0
		});
		_timers.id++;
		_timers.timerEntries.sort(function(a, b) {
			if (a.fire > b.fire) {
				return 1;
			}
			if (a.fire < b.fire) {
				return -1;
			}
			return 0;
		});
		return id;
	}, // setTimer
	cancelTimer: function(id) {
		for (var i=0; i<_timers.timerEntries.length; i++) {
			if (_timers.timerEntries[i].id == id) {
				_timers.timerEntries.splice(i, 1);
				return;
			}
		}
	} // cancelTimer
}

function cancelInterval2(id) {
	_timers.cancelTimer(id);
}

function cancelTimeout2(id) {
	_timers.cancelTimer(id);
}

function setInterval2(callback, interval) {
	return _timers.setTimer(callback, interval, true);
}

function setTimeout2(callback, interval) {
	return _timers.setTimer(callback, interval, false);
}

require("loop");