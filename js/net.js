// https://www.hacksparrow.com/tcp-socket-programming-in-node-js.html
function _callC(command) {
	
}
var _sockets= {};

var net = {
	Socket: function(options) {
		var sockfd;
		if (options && options.sockfd !== undefined) {
			sockfd = options.sockfd;
		} else {
			sockfd = OS.socket().sockfd;
		}
		var ret = {
			_sockfd: sockfd,
			_onData: null,
			_onClose: null,
			_onConnect: null,
			_onEnd: null,
			listening: false,
			connecting: false,
			remoteAddress: null,
			remotePort: null,
			localPort: null,
			write: function(data) {
				_callC("write", data);
			},
			end: function(data) {
				_callC("write", data);
			},
			on: function(eventType, callback) {
				if (eventType == "data") {
					this._onData = callback;
					return;
				}
				if (eventType == "close") {
					this._onClose = callback;
					return;
				}
				if (eventType == "connect") {
					this._onConnect = callback;
					return;
				}
				if (eventType == "end") {
					this._onEnd = callback;
					return;
				}
			}, // on
			/**
			 * options contains:
			 * port
			 * host
			 * localAddress
			 * localPort
			 * family
			 */
			connect: function(options, connectListener) {
				this.connecting = true;
				this.on("connect", connectListener);
				var connectRc = OS.connect({sockfd: this._sockfd, port: options.port, address: options.address});
			} // connect
		}; // return
		_sockets[sockfd] = ret;
		return ret;
	}, // function Socket
	createServer: function(acceptCallback) {
		// Create a socket
		var sock = new this.Socket();
		sock.listening = true;
		sock.on("connect", acceptCallback);
		return {
			listen: function(port) {
				sock.localPort = port;
				OS.bind({sockfd: sock._sockfd, port: port});
				OS.listen({sockfd: sock._sockfd});
				// Listen on the port
			} // listen
		} // return
		
	}, // createServer
	connect: function(options, connectCallback) {
		var newSocket = new this.Socket();
		_sockets[newSocket._sockfd] = newSocket;
		newSocket.connect(options, connectCallback);
		return newSocket;
	}

};

function testServer() {
	var server = net.createServer(function(sock) {
		console.log("A new connection was received!");
		log("New connection socket is " + JSON.stringify(sock));
		sock.on("data", function(data) {
			log("We have received new data over the socket! :" + data.toString());
		});
	});
	server.listen(8001);
}

function testClient() {
	var address = "54.175.219.8"; // httpbin.org
	var client = net.connect({address: address, port: 80}, function() {
		log("We have connected!");
	});
}

testClient();

/**
 * Primary loop that processes events.
 * @returns
 */
function loop() {
	// Process the file descriptors for sockets.  We build an array that contains
	// the set of file descriptors corresponding to the sockets that we wish to read from.
	var readfds = [];
	var writefds = [];
	for (var sock in _sockets) {
		readfds.push(_sockets[sock]._sockfd);
		if (_sockets[sock].connecting) {
			writefds.push(_sockets[sock]._sockfd);
		}
	} // End of each socket id.
	
	// Invoke select() to see if there is any work to do.
	var selectResult = OS.select({readfds: readfds, writefds: writefds, exceptfds:[]});
	//console.log("selectResult: " + JSON.stringify(selectResult));
	
	// Process the write sockets
	for (var i=0; i<selectResult.writefds.length; i++) {
		var currentSocketFd = selectResult.writefds[i];
		var currentSock = _sockets[selectResult.writefds[i]];
		if (currentSock.connecting) {
			// Aha!!  We had a connecting socket and now we can write ... that means we are connected!
			currentSock.connecting = false;
			if (currentSock._onConnect) {
				currentSock._onConnect()
			}
		} // For each socket that is able to write and was in connecting state.
	} // For each socket that is able to write
	
	// Process each ready to read file descriptor in turn
	for (var i=0; i<selectResult.readfds.length; i++) {
		var currentSocketFd = selectResult.readfds[i];
		log("working on read of: " + currentSocketFd);
		var currentSock = _sockets[selectResult.readfds[i]];
		
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
			var myData = new Buffer(64);
			var recvSize = OS.recv({sockfd: currentSock._sockfd, data: myData});
			log("Result from recv: " + recvSize);
			if (recvSize === 0) {
				if (currentSock._onEnd) {
					currentSock.onEnd();
				}
				if (currentSock._onClose) {
					currentSock.onClose();
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
setInterval(loop, 2000);