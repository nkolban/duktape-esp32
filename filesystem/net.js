// https://www.hacksparrow.com/tcp-socket-programming-in-node-js.html
/* globals _sockets, OS, module */
/**
 * Expected environment:
 * * A global array called _sockets should exist which contains the sockets.
 * 
 * Prereq modules:
 * * none
 * 
 */
var moduleSSL = ESP32.getNativeFunction("ModuleSSL");
if (moduleSSL === null) {
	log("Unable to find ModuleSSL");
	module.exports = null;
	return;
}

var internalSSL = {};
moduleSSL(internalSSL);

var net = {
	//
	// net.Socket(options)
	// The options parameter is an optional object which can contain:
	// * sockfd - A numeric socket fd of an open socket.
	//
	// connect - Connect to a target.
	// getFD - Return the underlyng file descriptor.
	// on - Register an event handler
	// - close
	// - data
	// - end
	// write - Write data to a target.

	Socket: function(options) {
		var sockfd;
		if (options && options.sockfd !== undefined) {
			sockfd = options.sockfd;
		} else {
			// hello
			sockfd = OS.socket().sockfd;
		}
		var ret = {
			_onData: null,
			_onClose: null,
			_onConnect: null,
			_onEnd: null,
			_note: null,
			_createTime: new Date().getTime(), // When the socket was created
			listening: false,
			connecting: false,
			remoteAddress: null,
			remotePort: null,
			localPort: null,
			
			//
			// write - Write data down the socket.  If we are using SSL, then write using the
			// SSL routines otherwise write using the OS send API.
			//
			write: function(data) {
				if (this.hasOwnProperty("dukf_ssl_context")) {
					return internalSSL.write(this.dukf_ssl_context, data);
				} else {
					var sendRc = OS.send({sockfd: sockfd, data: data});
					if (sendRc < 0) {
						throw new Error("Underlying send() failed");
					}
					return sendRc;
				}
			}, // write
			//
			// on - Register events.
			//
			on: function(eventType, callback) {
				if (eventType === "close") {
					this._onClose = callback;
					return;
				}
				if (eventType === "connect") {
					this._onConnect = callback;
					return;
				}
				if (eventType === "data") {
					this._onData = callback;
					return;
				}
				if (eventType === "end") {
					this._onEnd = callback;
					return;
				}
			}, // on
/**
 * connect: Connect to a partner socket
 * options contains:
 * * port
 * * host
 * * useSSL
 */
			//
			// connect
			//
			connect: function(options, connectListener) {
				this.connecting = true;
				this.on("connect", connectListener);
				var connectRc = OS.connect({
					sockfd: sockfd,
					port: options.port,
					address: net.getByName(options.address)
				});
				if (connectRc < 0) {
					throw new Error("Underlying connect() failed");
				}
				this.remotePort = options.port;
				this.remoteAddress = options.address;
				
				// If we are using SSL then create the SSL structures ...
				if (options.useSSL === true) {
					this.dukf_ssl_context = internalSSL.create_dukf_ssl_context(options.address, sockfd);
				}
			}, // connect
			
			//
			// close
			//
			// Here we flag the socket as ended.  This means we close the socket and delete it from
			// the list of known sockets.  This should be fine as we shouldn't ever try and read
			// from it or write from it again.
			end: function(data) {
				if (data !== undefined) {
					this.write(data);
				}
				OS.shutdown({sockfd: sockfd});
				OS.close({sockfd: sockfd});
				delete _sockets[sockfd];
			},
			setNote: function(text) {
				this._note = text;
			},
			//
			// getFD
			//
			getFD: function() {
				return sockfd;
			} // getFD
		}; // ret object
		_sockets[sockfd] = ret;
		return ret;
	}, // function Socket
	
	//
	// net.createServer(...)
	//
	createServer: function(connectionListener) {
		var sock = new this.Socket(); // Create a new socket
		sock.listening = true;
		sock.on("connect", connectionListener);
		return {
			listen: function(port) {
				sock.localPort = port;
				if (OS.bind({sockfd: sock.getFD(), port: port}) != 0) {
					throw new Error("Underlying bind() failed");
				}
				if (OS.listen({sockfd: sock.getFD()}) !=0) {
					throw new Error("Underlying listen() failed");
				}
				// Listen on the port
			} // listen
		}; // return
		
	}, // createServer
	
	//
	// connect(...)
	//
	connect: function(options, connectCallback) {
		var newSocket = new this.Socket();
		newSocket.connect(options, connectCallback);
		return newSocket;
	}, // connect
	
	//
	// closeAll
	//
	closeAll: function() {
		// force the close of ALL sockets ...
		while(_sockets.length > 0) {
			OS.shutdown({sockfd: _sockets[0].getFD()});
			OS.close({sockfd: _sockets[0].getFD()});
			delete _sockets[_sockets[0].getFD()];
		}
	}, // closeAll
	
	//
	// getByName()
	//
	// Return a string representation of the IP address of the address or null
	// on an error.  Will also parse regular IP addresses.
	getByName: function(address) {
		//return OS.gethostbyname(address);
		return OS.getaddrinfo(address);
	} // getByName

}; // End of module net

module.exports = net;