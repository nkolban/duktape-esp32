# Streams
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