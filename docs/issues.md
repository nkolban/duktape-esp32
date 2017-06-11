## mg_broadcast
When working with Mongoose, it appears we have some issues using mg_broadcast.  We need to investigate.

When using mg_broadcast, it appears that we have a send/receive pair that work on parallel threads/co-routines.
When mg_broadcast is called it writes data down a socket and then reads a response back from the same socket.  For
example:

```
send(sock0, data, length);
recv(sock0, &someData, 1);
```

So where does this socket come from?  The answer is that it is one half of a pair of sockets that have
been previously created.  When Mongoose starts up, it creates a listener socket that listens for an
incoming connection.  It then creates a sender socket that connects to the listener socket.  So we end up with
a socket pair INSIDE the ESP32.

Those two sockets are held within the Mongoose mgr object in the variables:

```
ctl[0] -> sender socket
ctl[1] -> receiver socket
```

The socket pair is created with a call to mg_socketpair() to create SOCK_DGRAM based sockets.  The logic is as 
follows:

```
int mg_socketpair(sock_t sp[2], int sock_type) {
  union socket_address sa;
  sock_t sock;
  socklen_t len = sizeof(sa.sin);
  int ret = 0;

  sock = sp[0] = sp[1] = INVALID_SOCKET;

  (void) memset(&sa, 0, sizeof(sa));
  sa.sin.sin_family = AF_INET;
  sa.sin.sin_port = htons(0);
  sa.sin.sin_addr.s_addr = htonl(0x7f000001); /* 127.0.0.1 */

  if ((sock = socket(AF_INET, sock_type, 0)) == INVALID_SOCKET) {
  } else if (bind(sock, &sa.sa, len) != 0) {
  } else if (sock_type == SOCK_STREAM && listen(sock, 1) != 0) {
  } else if (getsockname(sock, &sa.sa, &len) != 0) {
  } else if ((sp[0] = socket(AF_INET, sock_type, 0)) == INVALID_SOCKET) {
  } else if (connect(sp[0], &sa.sa, len) != 0) {
  } else if (sock_type == SOCK_DGRAM &&
             (getsockname(sp[0], &sa.sa, &len) != 0 ||
              connect(sock, &sa.sa, len) != 0)) {
  } else if ((sp[1] = (sock_type == SOCK_DGRAM ? sock
                                               : accept(sock, &sa.sa, &len))) ==
             INVALID_SOCKET) {
  } else {
    mg_set_close_on_exec(sp[0]);
    mg_set_close_on_exec(sp[1]);
    if (sock_type == SOCK_STREAM) closesocket(sock);
    ret = 1;
  }

  if (!ret) {
  	printf("mg_socketpair debug B - failed\n");
    if (sp[0] != INVALID_SOCKET) closesocket(sp[0]);
    if (sp[1] != INVALID_SOCKET) closesocket(sp[1]);
    if (sock != INVALID_SOCKET) closesocket(sock);
    sock = sp[0] = sp[1] = INVALID_SOCKET;
  }

  return ret;
}
```

The core of the algorithm is as follows:
```
socket_address sa;
sa.port   = 0;
sa.s_addr = 127.0.0.1;
sp[1] = socket(AF_INET, SOCK_DGRAM, 0); // Fail if INVALID_SOCKET
bind(sp[1], &sa.sa, len);               // Fail if != 0
getsockname(sp[1], &sa.sa, &len);       // Fail if != 0
sp[0] = socket(AF_INET, SOCK_DGRAM, 0); // Fail if INVALID_SOCKET
connect(sp[0], &sa.sa, len);            // Fail if != 0
getsockname(sp[0], &sa.sa, &len);       // Fail if != 0
connect(sp[1], &sa.sa, len);            // Fail if != 0

```