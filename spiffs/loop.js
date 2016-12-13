/* globals _sockets, OS, log, Buffer, require */
var net = require("net");
/**
 * Primary loop that processes events.
 * @returns
 */
function loop() {
	// Process the file descriptors for sockets.  We build an array that contains
	// the set of file descriptors corresponding to the sockets that we wish to read from.
	var readfds = [];
	var writefds = [];
	var exceptfds = [];
	
	// Loop through each of the sockets and determine if we are going to work with them.
	for (var sock in _sockets) {
		readfds.push(_sockets[sock]._sockfd);
		if (_sockets[sock].connecting) {
			writefds.push(_sockets[sock]._sockfd);
		}
		exceptfds.push(_sockets[sock]._sockfd);
	} // End of each socket id.
	
	// Invoke select() to see if there is any work to do.
	var selectResult = OS.select({readfds: readfds, writefds: writefds, exceptfds: exceptfds});
	//console.log("selectResult: " + JSON.stringify(selectResult));
	
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
		log("working on read of: " + currentSocketFd);
		currentSock = _sockets[selectResult.readfds[i]];
		
		// We now have the object that represents the socket.  If it is a listening
		// socket ... that means it is a server and we should accept a new client connection.
		if (currentSock.listening) {
			var acceptData = OS.accept({sockfd: currentSock._sockfd});
			log("We accepted a new client connection: " + JSON.stringify(acceptData));
			var newSocket = new net.Socket({sockfd: acceptData.sockfd});
			_sockets[newSocket._sockfd] = newSocket;
			
			newSocket.on("connect", currentSock._onConnect);
			if (newSocket._onConnect) {
				newSocket._onConnect(newSocket);
			}
		} // Socket was a server socket
		else {
			// We need to read data from it!
			// We have a socket in currentSocket that had data ready to be read from it.  Now we
			// read the data from that socket.
			var myData = new Buffer(512);
			var recvSize = OS.recv({sockfd: currentSock._sockfd, data: myData});
			log("Result from recv: " + recvSize);
			if (recvSize === 0) {
				if (currentSock._onEnd) {
					currentSock._onEnd();
				}
				if (currentSock._onClose) {
					currentSock._onClose();
				}
				OS.close({sockfd: currentSock._sockfd});
				// Now that we have closed the socket ... we can remove it from
				// our cache list.
			    delete _sockets[currentSock._sockfd];
			}  // We received no data.
			else {
				if (currentSock._onData) {
					currentSock._onData(myData.slice(0, recvSize));
				}
			}
		} // Data available and socket is NOT a server
	} // For each socket that is able to read ... 
} // loop

setInterval(loop, 100);
log("Major function \"loop()\" registered");