/* globals Duktape, log, DUKF, require */
/* exported _sockets, cancelInterval, cancelTimeout, setInterval, setTimeout, _loop */
/*
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
*/

StringUtils = {
	endsWith: function(subjectString, searchString, position) {
      if (typeof position !== 'number' || !isFinite(position) || Math.floor(position) !== position || position > subjectString.length) {
        position = subjectString.length;
      }
      position -= searchString.length;
      var lastIndex = subjectString.lastIndexOf(searchString, position);
      return lastIndex !== -1 && lastIndex === position;		
	},
	startsWith: function(subjectString, searchString, position) {
		position = position || 0;
		return subjectString.substr(position, searchString.length) === searchString;
	}
};

Duktape.modSearch = function(id, require, exports, module) {
	DUKF.gc();
	log("Module: require(\"" + id + "\") loading \"" + id + "\"");
	var name = id;
	if (!StringUtils.endsWith(id, ".js")) {
		name += ".js";
	}
	/*
	if (!id.endsWith(".js")) {
		name += ".js";
	}
	*/
	module.filename = name;
	return DUKF.loadFile(name);
	//var data = DUKF.loadFile(name);
	//log(data);
	//return data;
}; // Duktape.modSearch


//ESP32.setLogLevel("*", "debug");

// Create the globals
var _isr_gpio = {};

var _sockets= {};

var _timers = {
	nextId: 1,
	timerEntries: [],
	setTimer: function(callback, interval, isInterval) {
		var id = _timers.nextId;
		_timers.timerEntries.push({
			id: id,
			callback: callback,
			fire: new Date().getTime() + interval,
			interval: isInterval?interval:0
		});
		_timers.nextId++;
		_timers.sort();
		log("Added a new timer: id=" + id + " due to fire in " + interval);
		return id;
	}, // setTimer
	// Sort the array of timer entries such that the one which fires soonest is at the
	// start of the array.  This way, we only need to check the first entry to determine
	// if we need to fire a timer.  If now < the fire time of the first entry, then there
	// is nothing further to do since all the timers after this are later.
	sort: function() {
		_timers.timerEntries.sort(function(a, b) {
			if (a.fire > b.fire) {
				return 1;
			}
			if (a.fire < b.fire) {
				return -1;
			}
			return 0;
		});
	}, // sort
	cancelTimer: function(id) {
		for (var i=0; i<_timers.timerEntries.length; i++) {
			if (_timers.timerEntries[i].id == id) {
				_timers.timerEntries.splice(i, 1);
				return;
			}
		}
	} // cancelTimer
};

function cancelInterval(id) {
	_timers.cancelTimer(id);
}

function cancelTimeout(id) {
	_timers.cancelTimer(id);
}

function setInterval(callback, interval) {
	return _timers.setTimer(callback, interval, true);
}

function setTimeout(callback, interval) {
	return _timers.setTimer(callback, interval, false);
}

var _loop = require("loop.js");