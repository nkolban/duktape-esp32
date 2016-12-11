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
var net = {
	//
	// net.Socket(options)
	// The options parameter is an optional object which can contain:
	// * sockfd - A numeric socket fd of an open socket.
	//
	// connect - Connect to a target.
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
			//
			// write - Write data down the socket.
			//
			write: function(data) {
				OS.send({sockfd: this._sockfd, data: data});
			},
			//
			// on - Register events.
			//
			on: function(eventType, callback) {
				if (eventType === "data") {
					this._onData = callback;
					return;
				}
				if (eventType === "close") {
					this._onClose = callback;
					return;
				}
				if (eventType === "connect") {
					this._onConnect = callback;
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
			 * port
			 * host
			 * localAddress
			 * localPort
			 * family
			 */
			//
			// connect
			//
			connect: function(options, connectListener) {
				this.connecting = true;
				this.on("connect", connectListener);
				OS.connect({
					sockfd: this._sockfd,
					port: options.port,
					address: options.address});
			}, // connect
			//
			// close
			//
			end: function(data) {
				if (data !== undefined) {
					this.write(data);
				}
				OS.close({sockfd: this._sockfd});
			}
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
				if (OS.bind({sockfd: sock._sockfd, port: port}) != 0) {
					throw new Error("Underlying bind failed");
				}
				OS.listen({sockfd: sock._sockfd});
				// Listen on the port
			} // listen
		}; // return
		
	}, // createServer
	
	//
	// net.connect(...)
	//
	connect: function(options, connectCallback) {
		var newSocket = new this.Socket();
		_sockets[newSocket._sockfd] = newSocket;
		newSocket.connect(options, connectCallback);
		return newSocket;
	} // connect

}; // End of module net

module.exports = net;

/*
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
*/