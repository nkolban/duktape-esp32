# BADLY OUT OF DATE

----

# Working with HTTP
```
function handleRequest(request, response) {
	// The request contains details of the incoming request.
	// The response describes the response to be returned.
}

var server = http.createServer(handleRequest);
server.listen(port);
```

The properties and functions on request include:
* `headers`
* `method`
* `url`

The functions on response include:
* `end`
* `setHeader`
* `statusCode`
* `write`
* `writeHead`


When we wish to send an outbound HTTP request we will call 

`http.get(options, callback)`

or generically:

`http.request(options [,callback])`



The `options` describe the target of the request and at a minimum will contain:

* `host`
* `path`

The callback is a function that will be invoked with a response parameter:

`callback(response)`

The `response` will be a stream based object and hence have:
* `on("data", callback)`
* `on("end", callback)`
* `read()`

It will also have properties:

* `statusCode`

Thinking about the implementation of our solution, a response from the HTTP server can come at any time
and hence we have to add the response as an event to be processed by JS when it can.  Now we have the issue
of "correlation".  The good news is that we are using Mongoose to make our request and mongoose has a property
used for user data that can be used to hold the correlation.  So what should we save there?  The answer would
appear to be a reference to the "response" object passed in the original callback.  So how do we do that?


Let us imagine that an `http.get()` is implemented loosely as follows:

```
function get(callback) {
   create a stream;
   call the callback passing in the stream.reader;
   call Mongoose to make the request passing it the stream.writer;
}
```

When an async response arrives at mongoose, it knows the stream.writer but can NOT call it directly.  Why not?  Because it
isn't allowed.  Instead it must place an event on the event queue for JS to call it when it can.


