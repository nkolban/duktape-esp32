/**
 * https://nodejs.org/api/stream.html
 * @returns
 */
/*
#Streams
A writer can have data written to it.
A writer can be told that the writes have completed.

A reader can have data read from it.
A reader can have a 'data' event handler that is called when new data is written.
A reader can have a 'end' event handler that is called when there is no new data to be written.


Imagine the following:
```
function stream() {
	var buffer ... buffer to hold the data;
	var end = false;
	var writer = {
	   write: function(data) {
	      if (reader._dataCallbacks.length > 0) {
	         foreach(reader._dataCallback -> call with data);
	      } else {
	         append buffer with data;
	      }
	   };
	   end: function(data) {
	   	 end = true;
	   	 for each reader.endCallbacks -> call;
	   }
	};
	var reader = {
		_dataCallbacks: [],
		_endCallbacks: [],
		read: function() {
		},
		on(event, callback) {
			if (event == "data") {
				_dataCallbacks.push(callback);
				if (buffer.length >0) {
					callback(buffer);
					emptyBuffer;
				}
			}
			if (event == "end") {
				_endCallbacks.push(callback);
				if (end) {
				   callback();
				}
			}
		}
	}
}
```
*/
function streams() {
	var dataBuffer = null;
	var dataBufferUsed = 0;
	var end = false;
	var readerCallback = null;
	var endCallback = null;
	var writer = {
		write: function(data) {
			if (typeof data == "string") {
				data = new Buffer(data);
			}
			if (readerCallback !== null) {
				readerCallback(data);
			} else {
				if (dataBuffer === null) {
					dataBuffer = new Buffer(1000);
				}
				data.copy(dataBuffer, dataBufferUsed);
				dataBufferUsed += data.length;
			}
		}, // write()
		end: function(data) {
			if (data !== null && data !== undefined) {
				this.write(data);
			}
			end = true;
			if (endCallback) {
				endCallback();
			}
		} // end()
	};
	
	var reader = {
		read: function() {
			if (dataBufferUsed === 0) {
				return new Buffer(0);
			}
			var tempBuffer = new Buffer(dataBufferUsed);
			dataBuffer.copy(tempBuffer, 0, 0, dataBufferUsed);
			dataBufferUsed = 0;
			return tempBuffer;
		}, // read
		on: function(event, callback) {
			if (event == "data") {
				readerCallback = callback;
				if (dataBufferUsed > 0) {
					var tempBuffer = new Buffer(dataBufferUsed);
					dataBuffer.copy(tempBuffer, 0, 0, dataBufferUsed);
					readerCallback(tempBuffer);
					dataBufferUsed = 0;
				}
			} // data
			if (event == "end") {
				endCallback = callback;
				if (end) {
					callback();
				}
			} // end
		} // on

	}; // reader()
	return {
		reader: reader,
		writer: writer
	};
}

function eventCallback(callbackType, contextData, appData) {
   console.log("eventCallback called! \nContextData:" + contextData + "\nappData: " + appData +"\n");
   console.log("status: " + appData.status + "\n");
   console.log("body: " + appData.body + "\n");
   console.log("As JSON: " + JSON.stringify(appData) + "\n");
   var s = new streams();
   var response = s.reader;
   response.statusCode = appData.status;
   contextData(response);
   s.writer.write(appData.body);
   s.writer.end();
}

var http = {
	_counter: 1,
	_stash: {},
	request: function(options, responseCallback) {
		// Invoke Mongoose!
		this._stash[this._counter] = responseCallback;
		this._counter++;
		var uri = "http://" + options.host + options.path;
		MONGOOSE.request(uri, responseCallback);
	}
};

http.request("http://httpbin.org/get", function(response) {
	console.log("Response callback called!: " + response + "\n");
	response.on("data", function(d) {
		console.log("And here is the data! " + d + "\n");
	});
	console.log("Status is " + response.statusCode + "\n");
});