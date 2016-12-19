#Memory on Web Sockets
In this issue we have the story that when we start ESP32-Duktape with the built in WebServer,
we have a free heap of `44000` bytes.  After we load `test_ws.js` we are out of memory.

The heap values are:

Initial break point 37000
After break: 33000

Ahhh ... every time I run the eval() to look at the debug output, the free heap drops by 3.5K!!!


Startup ESP32-Duktape

Load the WS test program:

```
$ curl 192.168.1.99:8000/run/test_ws.js
```

Attach the debugger.  Step through with the "heapSize" logging eval:

```
ESP32.getState().heapSize
```