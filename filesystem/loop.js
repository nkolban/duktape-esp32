/*
 * Master loop of ESP32-Duktape
 * This is the loop function that is called as often as possible within the ESP32-Duktape
 * environment.  It is primarily responsible for:
 * 
 * * Checking to see if timers have expired.
 * * Polling network I/O to see if network actions are needed.
 */
/* globals _sockets, OS, log, Buffer, require, ESP32, _timers, module, DUKF */
var net = require("net.js");
var internalSSL = {};
var moduleSSL = ESP32.getNativeFunction("ModuleSSL");
if (moduleSSL === null) {
	log("Unable to find ModuleSSL");
} else {
	moduleSSL(internalSSL);
}


/**
 * Primary loop that processes events.
 * @returns
 */
function loop() {
	// Timer examination
	//
	// Handle a timer having expired ... we have a global variable called _timers.  This variable
	// contains an array called timerEntries.  Each entry in timerEntries contains:
	// {
	//    id:       The ID of the timer.
	//    callback: The function to be invoked on the callback.
	//    fire:     The time when the timer expires
	//    interval: 0 for a single timeout and a value > 0 for an interval timer.
	// }
	//
	
	if (_timers.timerEntries.length > 0 && new Date().getTime() >= _timers.timerEntries[0].fire ) {
		//log("Processing timer fired for id: " + _timers.timerEntries[0].id);
		var timerCallback = _timers.timerEntries[0].callback;
		if (_timers.timerEntries[0].interval > 0) {
			_timers.timerEntries[0].fire = new Date().getTime() + _timers.timerEntries[0].interval;
			_timers.sort();
		} else {
			_timers.timerEntries.splice(0, 1);
		}
		timerCallback();
	} // End of check timers.
	
	// Network I/O Polling
	//
	// Process the file descriptors for sockets.  We build an array that contains
	// the set of file descriptors corresponding to the sockets that we wish to read from.
	var readfds   = [];
	var writefds  = [];
	var exceptfds = [];
	
	// Loop through each of the sockets and determine if we are going to work with them.
	for (var sock in _sockets) {
		if (_sockets.hasOwnProperty(sock)) {
			readfds.push(_sockets[sock].getFD());
			if (_sockets[sock].connecting) {
				writefds.push(_sockets[sock].getFD());
			}
			exceptfds.push(_sockets[sock].getFD());
		}
	} // End of each socket id.
	
	// Invoke select() to see if there is any work to do.
	var selectResult = OS.select({readfds: readfds, writefds: writefds, exceptfds: exceptfds});
	if (selectResult.readfds.length > 0 || selectResult.writefds.length > 0 || selectResult.exceptfds.length > 0) {
		log("selectResult: " + JSON.stringify(selectResult));
	}
	
	var currentSocketFd;
	var currentSock;
	
	// Process the write sockets
	for (var i=0; i<selectResult.writefds.length; i++) {
		currentSocketFd = selectResult.writefds[i];
		currentSock = _sockets[selectResult.writefds[i]];
		if (currentSock.connecting) {
			// Aha!!  We had a connecting socket and now we can write ... that means we are connected!
			currentSock.connecting = false;
			if (currentSock._onConnect) {
				currentSock._onConnect();
			}
		} // For each socket that is able to write and was in connecting state.
	} // For each socket that is able to write
	
	// Process each ready to read file descriptor in turn
	for (i=0; i<selectResult.readfds.length; i++) {
		currentSocketFd = selectResult.readfds[i];
		log("loop: working on ready to read of fd=" + currentSocketFd);
		currentSock = _sockets[selectResult.readfds[i]];
		
		// We now have the object that represents the socket.  If it is a listening
		// socket ... that means it is a server and we should accept a new client connection.
		if (currentSock.listening) {
			var acceptData = OS.accept({sockfd: currentSock.getFD()});
			log("We accepted a new client connection: " + JSON.stringify(acceptData));
			var newSocket = new net.Socket({sockfd: acceptData.sockfd});

			newSocket._createdFromFd = currentSock.getFD();	// For debugging purposes only
			
			newSocket.on("connect", currentSock._onConnect);
			if (newSocket._onConnect) {
				newSocket._onConnect(newSocket);
			}
		} // Socket was a server socket
		else {
			// We need to read data from the socket!
			// We have a socket in currentSocket that had data ready to be read from it.  Now we
			// read the data from that socket.
			var myData = new Buffer(512);
			var recvSize;
			if (currentSock.hasOwnProperty("dukf_ssl_context")) {
				recvSize = internalSSL.read(currentSock.dukf_ssl_context, myData);
			} else {
				recvSize = OS.recv({sockfd: currentSock.getFD(), data: myData});
			}
			log("Length of data from recv: " + recvSize);
			if (recvSize === 0) {
				if (currentSock._onEnd) {
					currentSock._onEnd();
				}
				if (currentSock._onClose) {
					currentSock._onClose();
				}
				OS.close({sockfd: currentSock.getFD()});
				if (currentSock.hasOwnProperty("dukf_ssl_context")) {
					internalSSL.free_dukf_ssl_context(currentSock.dukf_ssl_context);
				}
				// Now that we have closed the socket ... we can remove it from
				// our cache list.
			   delete _sockets[currentSock.getFD()];
			}  // We received no data.
			else if (recvSize > 0 ) { // Data size was > 0 .
				if (currentSock._onData) {
					currentSock._onData(myData.slice(0, recvSize));
				}
			} // Data size was > 0
			myData = null;
		} // Data available and socket is NOT a server
	} // For each socket that is able to read ... 
	
	// Garbage collection.
	DUKF.gc();
} // loop

log("Major function \"loop()\" registered");

module.exports = loop;