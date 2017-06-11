# Network processing.
Node.js has the concept of sockets.  See: https://nodejs.org/api/net.html

At a high level, we have the "net" object which has classes:

* Server
* listen(port)
* Socket

To create a server, we call `net.createServer(connectionListener)`.  The return from this is an instance
of Server.  At this point the server is NOT yet listening for new connections.  To start that process
we call the `server.listen(port)` function.

```
var server = new.createServer(function(sock) {
});
server.listen(9876);
```

Once `server.listen()` has been called, under the covers, a socket will be created, bound to the port and started
the listening process.  Now we have a long async pause ... and at some time in the future, we should assume that
a connection will arrive.

When a connection arrives, the new socket returned will be wrapped in a "Socket" instance and passed as a parameter
to the callback function supplied to `createServer`.

So how can we wait on a new socket arriving in C?  The obvious way is to call `accept()` which will block waiting for a
connection to arrive.  This could be started in a FreeRTOS task but what if we have multiple listening sockets.  We would have
to have a task for each connection and now we start to get grossly wasteful.  Another potential solution is to use
the `select()` system call.  This powerful call has many uses.
