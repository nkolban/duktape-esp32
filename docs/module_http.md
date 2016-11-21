# Module: http
The `http` module provides HTTP services.  It should be loaded with a require() call such as:

```
var http = require("http");
```

Once loaded, we can now perform HTTP based functions.  We can either send an HTTP request or become an
HTTP server.

## HTTP Server
We create a server object with a call to `http.createServer()`.  This function takes a
callback function which will be invoked when a new browser request is received.

```
var http = require("http");
//Create a server
var server = http.createServer(requestHandler);

//Lets start our server
server.listen(80, function(){
   console.log("Server listening\n");
});
```

The object returned from a call to `createServer()` is an HTTP server instance.  The server doesn't
actually start listening for incoming browser requests until a call to `listen()` is made.  This
function takes the local (to ESP32) port number of which it is to listen upon.

### HTTP Server request handler
The request handler function is registered when we create a server instance.  It is a callback
function that is invoked when a browser request is received.  It is responsible for providing the
request information sent by the browser and at the same time for providing a mechanism for a response
to be supplied.  The signature of the callback function is:

```
function requestHandler(request, response) {
   // Handler code here
}
```

The `request` parameter is a reference to an HTTPServerRequest object while the `response` parameter
is a reference to an HTTPServerResponse object.  Both these objects have functions that can be called
to work with their purposes.

### HTTPServerRequest object
This is the type of object passed into a `requestHandler` invoked when we receive a new browser
request.  It exposes:

* `url` - The URL (path) requested by the browser.
* `method` - The HTTP method requested (eg. "GET").


### HTTPServerResponse object
This is the type of object passed into a `requestHandler` invoked when we receive a new browser
request.  It exposes:

* `writeHead(statusCode, headers)` - Set the status code and headers for the final response.  If not called, then
the return status will be `200` with no headers.
* `write(data)` - Add data to be sent to the browser.
* `end(data)` - Indicate the end of the response we are sending to the browser.  Data will not be
actually sent to the browser until this call is made.

----

# Design
The initial design makes use of Mongoose as a Web server.  Mongoose is a set of open source libraries
that expose the services of a Web Server.  When a request to `listen()` is made on the server object,
an instance of Mongoose is started under the covers.  It is Mongoose that is listening for requests.