/**
 * References:
 * Writing web socket servers: https://developer.mozilla.org/en-US/docs/Web/API/WebSockets_API/Writing_WebSocket_servers
 * RFC6455 - The WebSocket Protocol - https://datatracker.ietf.org/doc/rfc6455/
 * 
 * Here is an example of a web socket initiation request from a client:
 * 
 * GET /chat HTTP/1.1
 * Host: example.com:8000
 * Upgrade: websocket
 * Connection: Upgrade
 * Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==
 * Sec-WebSocket-Version: 13
 * 
 * A response will be:
 * HTTP/1.1 101 Switching Protocols
 * Upgrade: websocket
 * Connection: Upgrade
 * Sec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=
 */

/* globals Buffer, log, require, Duktape, OS, module */

var OPCODE= {
	CONTINUATION: 0x0,
	TEXT_FRAME:   0x1,
	BINARY_FRAME: 0x2,
	CLOSE:        0x8,
	PING:         0x9,
	PONG:         0xA
};
var HTTP=require("http.js");
var connectionCallback = null;

/*
Frame format:  
	​​
	      0                   1                   2                   3
	      0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
	     +-+-+-+-+-------+-+-------------+-------------------------------+
	     |F|R|R|R| opcode|M| Payload len |    Extended payload length    |
	     |I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
	     |N|V|V|V|       |S|             |   (if payload len==126/127)   |
	     | |1|2|3|       |K|             |                               |
	     +-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
	     |     Extended payload length continued, if payload len == 127  |
	     + - - - - - - - - - - - - - - - +-------------------------------+
	     |                               |Masking-key, if MASK set to 1  |
	     +-------------------------------+-------------------------------+
	     | Masking-key (continued)       |          Payload Data         |
	     +-------------------------------- - - - - - - - - - - - - - - - +
	     :                     Payload Data continued ...                :
	     + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
	     |                     Payload Data continued ...                |
	     +---------------------------------------------------------------+

[0] - FIN 7
    - RSV1 6
    - RSV2 5
    - RSV4 4
    - Opcode 3:0
[1] - Mask 8
      payloadLen 7:0 < 126
[2] - Mask
[3] - Mask
[4] - Mask
[5] - Mask
[6] - Payload data

or

[0] - FIN 7
    - RSV1 6
    - RSV2 5
    - RSV4 4
    - Opcode 3:0
[1] - Mask 8
      payloadLen 7:0 = 126
[2] - payloadLen
[3] - payloadLen
[4] - Mask
[5] - Mask
[6] - Mask
[7] - Mask
[8] - Payload data

or

[0] - FIN 7
    - RSV1 6
    - RSV2 5
    - RSV4 4
    - Opcode 3:0
[1] - Mask 8
      payloadLen 7:0 = 127
[2] - payloadLen
[3] - payloadLen
[4] - payloadLen
[5] - payloadLen
[6] - payloadLen
[7] - payloadLen
[8] - payloadLen
[9] - payloadLen
[10] - Mask
[11] - Mask
[12] - Mask
[13] - Mask
[14] - Payload data

*/

/**
 * Build a WebSocket frame from the payload
 * @param payload The payload to send.
 * @param mask A boolean indicating whether we are to mask or not.
 * @returns The frame to transmit.
 */
function constructFrame(payload, mask) {
	if (typeof payload == "string") {
		payload = new Buffer(payload);
	}
	log("Data to construct is " + payload);
	var payloadLen = payload.length;
	var frameSize = 2 + payloadLen;
	if (mask) {
		frameSize += 4;
	}
	if (payloadLen >= 126 ) {
		frameSize +=2 ;
	}
	var frame = new Buffer(frameSize); // Allocate the buffer for the final frame.
	var i = 0;
	frame[i] = 0x80 | OPCODE.TEXT_FRAME; // FIN + size
	i++;
	
	if (payloadLen < 126) {
		frame[i] = 0x00 | payloadLen; // No mask
		i++;
	} else {
		frame[i] = 0x0 | 126; // No mask
		i++;
		frame[i] = (payloadLen & 0xff00) >> 8;
		i++;
		frame[i] = (payloadLen & 0xff);
		i++;
	}
	
	// Here we do mask processing to mask the data (if requested)
	if (mask) {
		var maskValue = new Buffer(4);
		var j;
		for (j=0; j<4; j++) {
			maskValue[j] = Math.floor(Math.random() * 256);
		}
		maskValue.copy(frame, i, 0);
		i+=4;
		
		payload.copy(frame, i, 0);
		for (j=0; j<payloadLen; j++) {
			frame[i] = frame[i] ^ maskValue[j%4];
			i++;
		}
	} else {
		payload.copy(frame, i, 0);
	}
	log("Frame size is " + frame.length);
	return frame;
}


function parseFrame(data) {
	// Data is a buffer
	var fin = (data[0] & 0x80) >> 7; // 1000 0000
	var opCode = (data[0] & 0x0f);
	var mask = (data[1] & 0x80) >> 7;
	var payloadLen = (data[1] & 0x7f); // 0b0111 1111
	var maskStart = 2;
	if (payloadLen == 126) {
		payloadLen = data[2] * 1<<8 + data[3];
		maskStart = 4;
	}
	if (payloadLen == 127) {
		payloadLen = data[2] * 1<<56 + data[3] * 1<<48 + data[4] * 1<< 40 + data[5] * 1<<32 +
		data[6] * 1<<24 + data[7] * 1<<16 + data[8] * 1<<8 + data[9];
		maskStart = 10;
	}
	var maskValue;
	var payloadStart;
	if (mask == 1) {
		maskValue = new Buffer(4);
		data.copy(maskValue, 0, maskStart, maskStart+4);		
		payloadStart = maskStart + 4;
		
	} else {
		payloadStart = maskStart;
	}
	
	// The payload now starts at payloadStart ... we are ready to start working on it.
	
	var payloadData = new Buffer(payloadLen);
	data.copy(payloadData, 0, payloadStart, payloadStart + payloadLen);
	// Copy the frame data into the final payloadData buffer.  We may have to unmask the
	// data.
	if (mask == 1) {
		var i;
		for (i=0; i<payloadLen; i++) {
			payloadData[i] = payloadData[i] ^ maskValue[i%4]; // Perform an XOR to unmask
		}
	}
	log("WS Frame: fin=" + fin + ", opCode=" + opCode + ", mask=" + mask + ", payloadLen=" + payloadLen + ", data=" + payloadData);
	return payloadData;
} // parseFrame



function requestHandler(request, response) {
	debugger;
	log("*****");
   log("***** We have received a new WS HTTP client request!");
   request.on("data", function(data) {
      log("WS HTTP Request handler: " + data);
   });
   function fail() {
      response.writeHead(400);
      response.end();
   }
   request.on("end", function() {
      log("WS HTTP request received:");
      log(" - method: " + request.method);
      log(" - headers: " + JSON.stringify(request.headers));
      if (request.method != "GET") {
      	log("Method is not GET");
      	fail();
      	return;
      }
      if (request.getHeader("Upgrade") !== "websocket") {
      	log("Upgrade header is not websocket");
      	fail();
      	return;
      }
      if (request.getHeader("Connection") !== "Upgrade") {
      	log("Connection header is not Upgrade");
      	fail();
      	return;
      }
      if (request.getHeader("Sec-WebSocket-Key") === null) {
      	log("No Sec-WebSocket-Key HTTP header");
      	fail();
      	return;
      }
      
      // If we are here, so far so good and we start to build a response:
      var headers = {
      	"Upgrade": "websocket",
      	"Connection": "upgrade",
      	"Sec-WebSocket-Accept": Duktape.enc("base64", OS.sha1(request.getHeader("Sec-WebSocket-Key") + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"))
      };
      response.writeHead(101, headers);
      headers = null;
      
      // We have now gone native!!! ... we are now working in RAW sockets ...

      if (connectionCallback !== null) {
   		var sock = request.getSocket();
      	var newConnection = {
      		on: function(eventType, callback) {
      			if (eventType == "message") {
      				sock.on("data", function(incomingFrame) {        	
      					log("Received WS data!");
   		         	var incomingData = parseFrame(incomingFrame);
   		         	callback(incomingData);
   		         });      				
      			} // eventType == message
      		}, // on
      		send: function(data) {
      			var frame = constructFrame(data, false);
            	sock.write(frame);
      		}, // send
      		close: function() {
      			response.end();
      		}
      	};
      	connectionCallback(newConnection);
      } // connectionCallback != null
   }); // Request on end.
} // requestHandler


function Server(port) {
	var server = HTTP.createServer(requestHandler);
	server.listen(port);
	var wsret = {
		// connection
		on: function(eventType, callback) {
			if (eventType == "connection") {
				connectionCallback = callback;
			}
		}, // on
	}; // wsret;
	return wsret;
} // Server

module.exports = {
	Server: Server
};