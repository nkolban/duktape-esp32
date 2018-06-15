#!/usr/bin/env node
"use strict";

var sport = require('serialport');
var fs = require('fs');
var mylog = console.log;
var dir = '../filesystem/app';

if(process.argv.length>2) dir=process.argv[2];
mylog('chdir to directory ' + dir);
process.chdir(dir);

function dostat(o) {
	var size = -1;
	console.log('Stat ' + o.path);
	try {
		var s = fs.lstatSync(o.path);
		size = s.size;
	} catch(e) {}

	return new Buffer([size&255,(size>>8)&255,(size>>16)&255,(size>>24)&255]);
}
function doread(o) {
	console.log('Read ' + o.path + ' ' + o.pos + ',' + o.len);
	var f = fs.readFileSync(o.path); // NO ATTEMPT AT EFFICIENCY...
	if(!f) return false;
	if(o.pos) f = f.slice(o.pos);
	if(f.length > o.len) f = f.slice(0, o.len);
	return f;
}

var port = false;

if(true) {
	var data = '';
	var cleart = false;
	port = new sport('/dev/ttyUSB1', {baudRate: 115200*2, rtscts: false});
	port.on('open', function () {
		mylog('Serial port opened');
	});
	port.on('error', function(err) {
		mylog('Error:', err);
	});
	port.on('close', function () {
		mylog('Serial port closed');
	});
	port.on('data', function(d) {
		if(cleart) {
			clearTimeout(cleart);
			cleart = false;
		}
		cleart = setTimeout(function() {
			if(data) {
				mylog('Cleared out some stale data...' + data.length);
				mylog(data);
				data = '';
			}
			cleart = false;
		}, 1000);

		data += d;
		if(data.indexOf('\0') > 0)
		{
//			mylog('Got complete packet!');
			var o = JSON.parse(data.slice(0,-1));
			mylog('cmd:' + o.cmd);
			var tosend = false;
			if(o.path)
			{
				while(o.path.slice(0,1) == '/')
					o.path = o.path.slice(1);
			}
			if(o.cmd == 'stat') tosend = dostat(o);
			if(o.cmd == 'read') tosend = doread(o);
			if(!tosend)
				tosend = new Buffer(0);
			var len = tosend.length;
			tosend = Buffer.concat([new Buffer([len&255, (len>>8)&255]), tosend]);
			port.write(tosend);
			data = '';
		}
	});
}

setInterval(function() {}, 1000); // idle pulse to keep from exiting?
