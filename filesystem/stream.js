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
 *        not expect any more data.  It has reached the end.  We ensure that this callback is
 *        never called more than once even if a writer should signal an end more than once.
 */
function stream() {
	var dataBuffer = null;
	var dataBufferUsed = 0;
	var endFlag = false; // Set to true when writer has flagged an end
	var readerCallback = null;
	var endCallback = null;

	var writer = {
		//
		// write
		//
		write: function(data) {
			if (data === null || data === undefined) {
				throw new Error("No data supplied for write");
			}
			if (endFlag) {
				throw new Error("Request to write to stream but already ended");
			}
			
			if (typeof data == "string") {
				data = new Buffer(data);
			}
			if (readerCallback !== null) {
				readerCallback(data);
			}
			// We don't have a consumer for the stream (yet) so we store the data until
			// we have a consumer.
			else {
				if (dataBuffer === null) {
					dataBuffer = new Buffer(1000); // FIX .. we need to be able to increase.
				}
				data.copy(dataBuffer, dataBufferUsed);
				dataBufferUsed += data.length;
			}
		}, // write()
		
		//
		// end
		//
		// If the writer has previously signled an end, don't signal a second
		// one.
		end: function(data) {
			if (endFlag) {
				return;
			}
			
			if (data !== null && data !== undefined) {
				this.write(data);
			}

			endFlag = true;
			
			if (endCallback) {
				endCallback();
			}
		} // end()
	};
	
	/**
	 * Define the reader.
	 */
	var reader = {
		//
		// read
		//
		// Read data that has been accumulated and we have no "data" handler.
		read: function() {
			if (dataBufferUsed === 0) {
				return new Buffer(0);
			}
			var tempBuffer = new Buffer(dataBufferUsed);
			dataBuffer.copy(tempBuffer, 0, 0, dataBufferUsed);
			dataBufferUsed = 0;
			return tempBuffer;
		}, // read
		
		//
		// on
		//
		// Register handlers for events sent by the writer.
		on: function(event, callback) {
			if (callback === null || callback === undefined) {
				throw new Error("No callback function supplied");
			}
			
			// Here the caller wishes to register a callback to receive data
			// we may have received and accumulated data BEFORE the caller has
			// requested it.  In this case, we must send the data that we have
			// stored up till now.
			if (event == "data") {
				readerCallback = callback; // Save the readerCallback which will be invoked on writes.
				if (dataBufferUsed > 0) {
					var tempBuffer = new Buffer(dataBufferUsed);
					dataBuffer.copy(tempBuffer, 0, 0, dataBufferUsed);
					readerCallback(tempBuffer);
					dataBuffer = null; // Release any storage we may have saved for a dataBuffer.
					dataBufferUsed = 0;
				}
			} // data
			
			// The caller wishes to register a callback to be notified when the writer
			// has claimed to have sent all the data.  We may have already been notified
			// so we may need to invoke the callback immediately.
			else if (event == "end") {
				endCallback = callback;
				if (endFlag) {
					endCallback();
				}
			} // end
			else {
				throw new Error("Unknown event type: "+ event);
			}
		} // on
	}; // reader()
	
	// Return the reader and writer.
	return {
		reader: reader,
		writer: writer
	};
} // stream

module.exports = stream;