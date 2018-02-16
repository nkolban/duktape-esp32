#!/usr/bin/env node
"use strict";

var net = require('net');
var fs = require('fs');

process.chdir('../filesystem/app');

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

var server = net.createServer(function(socket) {
	var data = '';
	console.log('Client connected');
	socket.on('data', function(d) {
		data += d;
		if(data.indexOf('\0') > 0)
		{
//			console.log('Got complete packet!');
			var o = JSON.parse(data.slice(0,-1));
//			console.log('cmd:' + o.cmd);
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
			socket.write(tosend);
			data = '';
		}
	});

	socket.on('close', function() {console.log('Client disconnected');});
});

server.listen(35555, '0.0.0.0');
