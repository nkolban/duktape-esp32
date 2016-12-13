#include <lwip/sockets.h>
#include <esp_log.h>
#include <string.h>
#include <errno.h>
#include <esp_wifi.h>
#include "esp32_duktape/duktape_event.h"
#include "sdkconfig.h"

#define PORT_NUMBER 8001

static char tag[] = "socket_server";

/**
 * Create a listening socket.  We then wait for a client to connect.
 * Once a client has connected, we then read until there is no more data
 * and log the data read.  We then close the client socket and start
 * waiting for a new connection.
 */
void socket_server(void *ignore) {
	struct sockaddr_in clientAddress;
	struct sockaddr_in serverAddress;

	// Create a socket that we will listen upon.
	int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock < 0) {
		ESP_LOGE(tag, "socket: %d %s", sock, strerror(errno));
		goto END;
	}

	// Bind our server socket to a port.
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddress.sin_port = htons(PORT_NUMBER);
	int rc  = bind(sock, (struct sockaddr *)&serverAddress, sizeof(serverAddress));
	if (rc < 0) {
		ESP_LOGE(tag, "bind: %d %s", rc, strerror(errno));
		goto END;
	}

	// Flag the socket as listening for new connections.
	rc = listen(sock, 5);
	if (rc < 0) {
		ESP_LOGE(tag, "listen: %d %s", rc, strerror(errno));
		goto END;
	}

	while (1) {
		// Listen for a new client connection.
		ESP_LOGD(tag, "Waiting for new client connection");
		socklen_t clientAddressLength = sizeof(clientAddress);
		int clientSock = accept(sock, (struct sockaddr *)&clientAddress, &clientAddressLength);
		if (clientSock < 0) {
			ESP_LOGE(tag, "accept: %d %s", clientSock, strerror(errno));
			goto END;
		}

// We now have a new client that is sending in JavaScript to evaluate.  We want
// to receive ALL the JavaScript before processing it.  Since we are memory sensitive,
// we can't just allocate a HUGE buffer.  So we need to allocate a small buffer, read in
// some data and keep increasing the buffer size as there is more data to be received.
		size_t totalAllocated =	1024;
		size_t sizeUsed = 0;
		char *data = malloc(totalAllocated);

		// Loop reading data.
		while(1) {
			ssize_t sizeRead = recv(clientSock, data + sizeUsed, totalAllocated-sizeUsed, 0);
			if (sizeRead < 0) {
				ESP_LOGE(tag, "recv: %d %s", sizeRead, strerror(errno));
				goto END;
			}
			if (sizeRead == 0) { // If sizeRead is 0, then we have reached the end.
				break;
			}
			sizeUsed += sizeRead;
// If we are running low on buffer into which to read more data from the socket, then
// we calculate the new buffer size and realloc the existing buffer.
			if ((totalAllocated-sizeUsed) < 512) {
				totalAllocated += 1024 - (totalAllocated - sizeUsed); // Should increase the totalAllocated free to 1024
				char *newPtr = realloc(data, totalAllocated);
				if (newPtr == NULL) { // It is possible we may not be able to reallocate!
					sizeUsed = 0;
					ESP_LOGD(tag, "Unable to realloc storage to %d", totalAllocated);
					break;
				}
				data = newPtr; // At this point we know we succesfully re-allocated.
			}
		} // Loop forever.

		// Finished reading data.
		//ESP_LOGD(tag, "Data read (size: %d) was: %.*s", sizeUsed, sizeUsed, data);
		if (sizeUsed > 0) {
			event_newCommandLineEvent(data, sizeUsed, 0);
		}
		free(data);
		close(clientSock);
	}
	END:
	vTaskDelete(NULL);
} // socket_server
