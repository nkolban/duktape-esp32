#Serial access
The ESP32 has three exposed serial (UART) ports called 0, 1 and 2.  A class called `Serial` has been provided to access these ports.
The ESP32 is highly configurable on its pin mappings but defaults are provided.

| UART  | RX Pin  | TX Pin  |
|-------|---------|---------|
| UART0 | GPIO 3  | GPIO 1  |
| UART1 | GPIO 9  | GPIO 10 |
| UART2 | GPIO 16 | GPIO 17 |

The Serial implementation is divided into two parts.  There is a low level underlying implementation
that provides stateless calls to interact with the ESP32 serial/UART environment.  These are exposed
as JS functions that take the port identifier as a parameter.  They are not exposed to the end users.
Layered on top of this is the notion of a SerialPort class that is created from a new instance of a
Serial class.  For example:

```
var Serial = require("Serial");
var serialPort = new Serial(1); // Here ... 1 means UART 1
```

From the SerialPort class we can now work with the specific serial port.  The SerialPort class
has methods to configure the port, write data to the port, explicitly read data from the port and
register an asynchronous event handler to be called when new data is available.

The asynchronous processing (as of 2017-01-01) is implemented by a polling loop utilizing an interval
timer.  Each timer the timer fires, it tries to read more data from the port and, if data is found,
invokes the callback.  This may be a sub-optimal implementation and alternatives could use either
UART interrupt handling or the FreeRTOS Queue technology built into ESP-IDF.

##Serial command processor
With the addition of serial support to ESP32-Duktape, we have the opportunity to add a serial command
processor.  What this will be is the ESP32 explicitly watching a serial input for incoming "commands"
and honoring those commands.  So what might a command be?  Possibilities include:

* REPL commands for immediate execution.
* Scripts for execution.
* Requests to report on diagnostics.

The current design is to think as the end point of the UART as an HTTP server to which we can send HTTP
requests (over serial).

The commands available so far are:

* run a script - X-Command: RUN
* get a list of files: X-Command: LIST
* save a file - X-Command: SAVE, X-Filename: <filename>


```
POST /run HTTP/1.1<CRLF>
Content-Length: <length><CRLF>
X-Command: RUN<CRLF>
<CRLF>
<payload data>
```

```
GET /list HTTP/1.1<CRLF>
Content-Length: <length><CRLF>
X-Command: LIST<CRLF>
<CRLF>
```

```
POST /save HTTP/1.1<CRLF>
Content-Length: <length><CRLF>
X-Command: SAVE<CRLF>
<CRLF>
<payload data>
```

and if ESP32-Duktape can parse this, we have input.
