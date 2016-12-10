// https://www.hacksparrow.com/tcp-socket-programming-in-node-js.html
function _callC(command) {
	
}
var _sockets= {};

/**
 * Given a line of text that is expected to be "\r\n" delimited, return an object that
 * contains the first line and the remainder.
 * @param data The input text to parse.
 * @returns An object that contains
 * {
 *    line: <The line of text up to the first \r\n>
 *    remainder: The remainder of the data after the first \r\n or the whole
 *       data if there is no first \r\n.
 * }
 */
function getLine(data) {
	var i = data.indexOf("\r\n");
	if (i==-1) {
		return {
			line: null,
			remainder: data
		}
	}
	return {
		line: data.substr(0, i),
		remainder: data.substr(i+2)
	}
} // getLine

/* Getline tests ...
var text = "1234567890\r\nabcdefghij\r\n";
log(JSON.stringify(getLine(text)));
text = "1234567890\r\n";
log(JSON.stringify(getLine(text)));
text = "1234567890";
log(JSON.stringify(getLine(text)));
*/


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
				OS.send({sockfd: this._sockfd, data: data});
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

/**
 * The HTTP Module.
 */
var http = {
	// Send an HTTP request.  The options object contains the details of the request
	// to be sent.
	// {
	//    address: <IP Address of target of request>
	//    port: <port number of target> [optional; default=80]
	//    path: <Path within the url> [optional; default="/"]
	// }
	//
	request: function(options, callback) {
		if (options.address === undefined) {
			log("http.request: No address set");
			return;
		}
		
		if (options.path === undefined) {
			options.path = "/";
		}


		if (options.port === undefined) {
			options.port = 80;
		}
		
		var sock = new net.Socket();
		var clientRequest = {
			_sock: sock	
		};
		callback(sock);
		var message = "";
		sock.connect(options, function() {
			// We are now connected ... send the HTTP message
			var requestMessage = "GET " + options.path + " HTTP/1.0\r\n" + 
			"User-Agent: ESP32-Duktape\r\n" +
			"\r\n";
			sock.write(requestMessage);
		});
		sock.on("data", function(data) {
			message += data.toString();
		});
		sock.on("end", function() {
			var STATE = {
				START: 1,
				HEADERS: 2,
				BODY: 3
			}
			log("HTTP session over ... data is: " + message);
			// We now have the WHOLE HTTP response message ... so it is time to parse the data
			var state = STATE.START;
			var line = getLine(message);
			var headers = {};
			while (line.line !== null) {
				if (state == STATE.START) {
					var splitData = line.line.split(" ");
					var httpStatus = splitData[1];
					log("httpStatus = " + httpStatus);
					// We found the start line ...
					state = STATE.HEADERS;
				} else if (state == STATE.HEADERS) {
					if (line.line.length == 0) {
						log("End of headers\n" + JSON.stringify(headers));
						state = STATE.BODY;
					} else {
						// we found a header
						var i = line.line.indexOf(":");
						var name = line.line.substr(0, i);
						var value = line.line.substr(i+2);
						headers[name] = value;
					}
					
				}
				
				log("New line: " + line.line);
				line = getLine(line.remainder);
			}
			log("Final remainder: " + line.remainder);
			log("Remainder length: " + line.remainder.length);
		});
		return clientRequest;
	}
}; // http


function testServer() {
	var server = net.createServer(function(sock) {
		log("A new connection was received!");
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
		client.on("data", function(data) {
			log("We have received new data over the socket! :" + data.toString());
		});
	});
}

function testHTTPClient() {
	var address = "54.175.219.8"; // httpbin.org
	var clientRequest = http.request({address: address, port: 80, path: "/get"}, function(response) {
		log("HTTP Response ready ...");
	});
}



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

ESP32.setLogLevel("*", "debug");
testHTTPClient();