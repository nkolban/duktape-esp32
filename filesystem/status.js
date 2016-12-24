(function() {
	log("Heap: " + ESP32.getState().heapSize);
	log("Sockets: " + JSON.stringify(_sockets));
	
	var counter = 0;
	for (var i in _sockets) {
		if (_sockets.hasOwnProperty(i)) {
			if (_sockets[i].listening) {
				log(" - socket: " + _sockets[i]._sockfd + ", listening, port: " + _sockets[i].localPort + ", creationTime: " + _sockets[i]._createTime);
			} else {
				log(" - socket: " + _sockets[i]._sockfd + ", creationTime: " + _sockets[i]._createTime);
			}
	
			counter++;
		}
	}
	log("Number of sockets being used: " + counter);
	log("Timers: " + JSON.stringify(_timers));
})();