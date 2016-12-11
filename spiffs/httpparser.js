/**
 * Parse HTTP requests and responses.
 * When we receive data we may receive either HTTP responses from previous requests
 * that we have sent or we may receive HTTP requests if we are acting as a Web Server.
 * This module provides a solution for parsing such data.
 * 
 * It is based on the notion of streams.
 * 
 * Here is a Response example:
 * var HTTPParser = require("httpparser");
 * function responseHandler(response) {
 * // Response contains:
 * // - headers
 * // - statusCode 
 *    on("data", function(data) {
 *    	// Handle data
 *    });
 *    on("end", function() {
 *       // handle end
 *    });
 * }
 * var parser = new HTTPParser("response", responseHandler);
 * // responseData read from socket ...
 * parser.write(responseData);
 * parser.end();
 * 
 * 
 * 
 * Here is a Request example:
 * var HTTPParser = require("httpparser");
 * function requestHandler(request) {
 * // Response contains:
 * // - headers
 * // - method
 *    on("data", function(data) {
 *    	// Handle data
 *    });
 *    on("end", function() {
 *       // handle end
 *    });
 * }
 * var parser = new HTTPParser("request", requestHandler);
 * // requestData read from socket ...
 * parser.write(requestData);
 * parser.end();
 */
/* globals require, log, module */

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
	if (i === -1) { // If we didn't find a terminator, then no lines and remainder is all.
		return {
			line: null,
			remainder: data
		};
	}
	return {
		line: data.substr(0, i),
		remainder: data.substr(i+2)
	};
} // getLine

var STATE = {
	START_REQUEST: 1,
	START_RESPONSE: 2,
	HEADERS: 3,
	BODY: 4
};
	
var Stream = require("stream");
function httpparser(type, handler) {
	var networkStream = new Stream();
	var httpStream = new Stream();
	var message = "";
	var path = "";
	
	handler(httpStream.reader);
	networkStream.reader.on("end", function() {
		log("HTTP parsing over ... data is: " + message);
		// We now have the WHOLE HTTP response message ... so it is time to parse the data
		var state;
		if (type === "request") {
			state = STATE.START_REQUEST;
		} else if (type === "response") {
			state = STATE.START_RESPONSE;
		} else {
			log("ERROR: Unknown type on httpparser: " + type);
		}

		var line = getLine(message);
		var headers = {};
		var method = null;
		while (line.line !== null) {
			if (state == STATE.START_RESPONSE) {
		// A header line is of the form <protocols>' '<code>' '<message>
//		                                   0          1         2					
				var splitData = line.line.split(" ");
				var httpStatus = splitData[1];
				log("httpStatus = " + httpStatus);
				// We have finished with the start line ...
				state = STATE.HEADERS;
			} // End of in STATE.START_RESPONSE
			else if (state == STATE.START_REQUEST) {
				var splitData = line.line.split(" ");
				method = splitData[0];
				path = splitData[1];
				log("http Method: " + method + ", path: " + path);
				// We have finished with the start line ...
				state = STATE.HEADERS;
			}
			else if (state == STATE.HEADERS) {
		// Between the headers and the body of an HTTP response is an empty line that marks the end of
		// the HTTP headers and the start of the body.  Thus we can find a line that is empty and, if it
		// is found, we switch to STATE.BODY.  Otherwise, we found a header line of the format:
		// <name>':_'<value>
				if (line.line.length === 0) {
					log("End of headers\n" + JSON.stringify(headers));
					state = STATE.BODY;
				} else {
				// we found a header
					var i = line.line.indexOf(":");
					var name = line.line.substr(0, i);
					var value = line.line.substr(i+2);
					headers[name] = value;
				}
			} // End of in STATE.HEADERS
			log("New line: " + line.line);
			line = getLine(line.remainder);
		} // End of we have processed all the lines. (end while)
		log("Final remainder: " + line.remainder);
		log("Remainder length: " + line.remainder.length);
		httpStream.reader.headers = headers;
		httpStream.reader.method = method;
		httpStream.reader.method = path;
		httpStream.writer.write(line.remainder);
		httpStream.writer.end();
	}); // networkStream reader on("end")
	
	networkStream.reader.on("data", function(data) {
		message += data.toString();
	});
	return networkStream.writer;
} // httpparser

module.exports = httpparser;
