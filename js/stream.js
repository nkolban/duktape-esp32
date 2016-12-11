/**
 * https://nodejs.org/api/stream.html
 * @returns
 */
/* globals Buffer, module */
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
/**
 * A stream object is created with
 * var myStream = new stream();
 * The result is an object that contains:
 * {
 *    reader: <A representation of a reader of data>
 *    writer: <A representation of a write of data>
 * }
 * 
 * The writer object contains:
 * write: Write some data.  The input can be either a Buffer or a String.
 * end: Flag the stream writing as complete optionally passing in some final data.
 * 
 * The reader object contains:
 * read: <read some data>
 * on: Register an event handler
 * - data: A callback that takes a Buffer parameter.  When called, new data is available
 *         and can be found in the passed in buffer.
 * - end: A callback (with no parameters) that indicates that the reader should
 *        not expect any more data.  It has reached the end.
 */
function stream() {
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
			if (data !== null) {
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

module.exports = stream;