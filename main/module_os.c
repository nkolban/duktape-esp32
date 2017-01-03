/**
 * Mapping from JavaScript (Duktape) to the underling OS (ESP-IDF)
 */
#if defined(ESP_PLATFORM)

#include <driver/gpio.h>
#include <esp_log.h>
#include <lwip/netdb.h>
#include <lwip/sockets.h>
#include <mbedtls/sha1.h>
#include <string.h>

#include "esp32_specific.h"
#include "sdkconfig.h"

#else // ESP_PLATFORM

#include <arpa/inet.h>
#include <netdb.h>
#include <openssl/sha.h>
#include <sys/socket.h>

#endif // ESP_PLATFORM

#include <errno.h>
#include <unistd.h>

#include "duktape.h"
#include "module_os.h"
#include "logging.h"


LOG_TAG("module_os");

/**
 * Accept an incoming client request.
 * [0] - Parms Object
 *  - sockfd - The socket fd.
 *
 * return:
 * {
 *    sockfd: <new socket fd>
 * }
 */
static duk_ret_t js_os_accept(duk_context *ctx) {
	LOGD(">> js_os_accept");
	if (!duk_is_object(ctx, -1)) {
		LOGE("js_os_accept: No parameters object found.");
		return 0;
	}
	if (!duk_get_prop_string(ctx, -1, "sockfd")) {
		LOGE("js_os_accept: No sockfd property found.");
		return 0;
	}
	int sockfd = duk_get_int(ctx, -1);
	LOGD(" About to call accept on %d", sockfd);
	int newSockfd = accept(sockfd, NULL, NULL);
	if (newSockfd < 0) {
		LOGE("Error with accept: %d: %d - %s", newSockfd, errno, strerror(errno));
		return 0;
	}
	//fcntl(newSockfd, F_SETFL, fcntl(newSockfd, F_GETFL, 0) | O_NONBLOCK); // Set the socket to be non blocking.
	duk_push_object(ctx);
	duk_push_int(ctx, newSockfd);
	duk_put_prop_string(ctx, -2, "sockfd");
	LOGD("<< js_os_accept: new socketfd=%d", newSockfd);
	return 1;
} // js_os_accept


/**
 * Bind a socket to an address
 * [0] - Params object
 * - port: The port to bind to.
 * - sockfd: The socket to bind.
 *
 * returns the bind rc;
 */
static duk_ret_t js_os_bind(duk_context *ctx) {
	LOGD(">> js_os_bind");
	if (!duk_is_object(ctx, -1)) {
		LOGE("js_os_bind: No parameters object found.");
		return 0;
	}
	if (!duk_get_prop_string(ctx, -1, "port")) {
		LOGE("js_os_bind: No port property found.");
		return 0;
	}
	int port = duk_get_int(ctx, -1);
	duk_pop(ctx);

	if (!duk_get_prop_string(ctx, -1, "sockfd")) {
		LOGE("js_os_bind: No sockfd property found.");
		return 0;
	}
	int sockfd = duk_get_int(ctx, -1);
	duk_pop(ctx);

	struct sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddr.sin_port = htons(port);

	LOGD("About to call bind on fd=%d with port=%d", sockfd, port);
	int rc = bind(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
	if (rc < 0) {
		LOGE("Error with bind: %d: %d - %s", rc, errno, strerror(errno));
	}
	duk_push_int(ctx, rc);
	LOGD("<< js_os_bind");
	return 1;
} // js_os_bind


/**
 * Close the socket.
 * [0] - Params object
 * - sockfd: The socket to close.
 *
 * There is no return code.
 */
static duk_ret_t js_os_close(duk_context *ctx) {
	LOGD(">> js_os_close");
	if (!duk_is_object(ctx, -1)) {
		LOGE("js_os_close: No parameters object found.");
		return 0;
	}
	if (!duk_get_prop_string(ctx, -1, "sockfd")) {
		LOGE("js_os_close: No sockfd property found.");
		return 0;
	}
	int sockfd = duk_get_int(ctx, -1);
	duk_pop(ctx);

	LOGD("About to close fd=%d", sockfd);
	int rc = close(sockfd);
	if (rc < 0) {
		LOGE("Error with close: %d: %d - %s", rc, errno, strerror(errno));
	}

	LOGD("<< js_os_close");
	return 0;
} // js_os_close


/**
 * Close the socket.
 * [0] - Params object
 * - sockfd: The socket to close.
 */
static duk_ret_t js_os_closesocket(duk_context *ctx) {
#if defined(ESP_PLATFORM)
	if (!duk_is_object(ctx, -1)) {
		LOGE("js_os_closesocket: No parameters object found.");
		return 0;
	}
	if (!duk_get_prop_string(ctx, -1, "sockfd")) {
		LOGE("js_os_closesocket: No sockfd property found.");
		return 0;
	}
	int sockfd = duk_get_int(ctx, -1);
	duk_pop(ctx);
	closesocket(sockfd);
	return 0;
#else /* ESP_PLATFORM */
	LOGE("js_os_closesocket not implemented.");
#endif /* ESP_PLATFORM */
} // js_os_closesocket


/**
 * Connect to a remote network partner.
 * The input is an options parameter object containing:
 * - sockfd - The socket file descriptor.
 * - address - The target address.
 * - port - The target port number.
 */
static duk_ret_t js_os_connect(duk_context *ctx) {
	int sockfd;
	int port;
	int connectRc;
	char *address;

	LOGD(">> js_os_connect");
	if (!duk_is_object(ctx, -1)) {
		LOGE("js_os_connect: No parameters object found.");
		return 0;
	}
	if (!duk_get_prop_string(ctx, -1, "sockfd")) {
		LOGE("js_os_connect: No sockfd property found.");
		return 0;
	}
	if (!duk_is_number(ctx, -1)) {
		LOGE("js_os_connect: Sockfd property is not a number.");
		return 0;
	}

	sockfd = duk_get_int(ctx, -1);
	duk_pop(ctx);

	if (!duk_get_prop_string(ctx, -1, "port")) {
		LOGE("js_os_connect: No port property found.");
		return 0;
	}
	if (!duk_is_number(ctx, -1)) {
		LOGE("js_os_connect: port property is not a number.");
		return 0;
	}

	port = duk_get_int(ctx, -1);
	duk_pop(ctx);

	if (!duk_get_prop_string(ctx, -1, "address")) {
		LOGE("js_os_connect: No address property found.");
		return 0;
	}
	if (!duk_is_string(ctx, -1)) {
		LOGE("js_os_connect: address property is not a sting.");
		return 0;
	}

	address = (char *)duk_get_string(ctx, -1);
	duk_pop(ctx);


	struct sockaddr_in serverAddress;
	serverAddress.sin_family = AF_INET;
	//serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddress.sin_port = htons(port);
	inet_pton(AF_INET, address, &serverAddress.sin_addr.s_addr);
	LOGD(" - About to connect fd=%d, address=%s, port=%d", sockfd, address, port);
	connectRc = connect(sockfd, (struct sockaddr *)&serverAddress, sizeof(serverAddress));
	if (connectRc != 0) {
		LOGE("Error with connect: %d: %d - %s", connectRc, errno, strerror(errno));
	}
	duk_push_int(ctx, connectRc);
	LOGD("<< js_os_connect: rc=%d", connectRc);
	return 1;
} // js_os_connect

/*
 * Retrieve the hostname for the address or null if not resolvable.
 * [0] - hostname
 */
static duk_ret_t js_os_gethostbyname(duk_context *ctx) {
	LOGD(">> js_os_gethostbyname");
	const char *hostname = duk_get_string(ctx, -1);
	LOGD(" - Looking up %s", hostname);
	struct hostent *hostent = gethostbyname(hostname);
	if (hostent == NULL) {
		LOGD(" - not found! h_errno = %d", h_errno);
		duk_push_null(ctx);
	} else {
		char retName[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, hostent->h_addr_list[0], retName, sizeof(retName));
		duk_push_string(ctx, retName);
	}
	LOGD("<< js_os_gethostbyname");
	return 1;
} // js_os_gethostbyname


/*
 * GPIO Functions
 */
#if defined(ESP_PLATFORM)
/*
 * Set the GPIO direction of the pin.
 * [0] - Pin number
 * [1] - Direction - 0=Input, 1=output
 */
static duk_ret_t js_os_gpioSetDirection(duk_context *ctx) {
	gpio_mode_t mode;
	gpio_num_t pinNum = duk_get_int(ctx, -2);

	int modeVal = duk_get_int(ctx, -1);
	if (modeVal == 1) {
		mode = GPIO_MODE_OUTPUT;
	} else {
		mode = GPIO_MODE_INPUT;
	}
	esp_err_t rc = gpio_set_direction(pinNum, mode);
	if (rc != 0) {
		LOGE("gpio_set_direction: %s", esp32_errToString(rc));
	}
	return 0;
} // js_os_gpioSetDirection


/*
 * Initialize the GPIO pin.
 * [0] - Pin number
 */
static duk_ret_t js_os_gpioInit(duk_context *ctx) {
	gpio_num_t pinNum = duk_get_int(ctx, -2);
	gpio_pad_select_gpio(pinNum);
	return 0;
} // js_os_gpioInit


/*
 * Set the GPIO level of the pin.
 * [0] - Pin number
 * [1] - level - true or false
 */
static duk_ret_t js_os_gpioSetLevel(duk_context *ctx) {
	uint32_t level;
	gpio_num_t pinNum = duk_get_int(ctx, -2);
	duk_bool_t levelBool = duk_get_boolean(ctx, -1);
	if (levelBool == 0) {
		level = 0;
	} else {
		level = 1;
	}
	esp_err_t rc = gpio_set_level(pinNum, level);
	if (rc != 0) {
		LOGE("gpio_set_level: %s", esp32_errToString(rc));
	}
	return 0;
} // js_os_gpioSetLevel





/*
 * Get the GPIO level of the pin.
 * [0] - Pin number
 */
static duk_ret_t js_os_gpioGetLevel(duk_context *ctx) {
	gpio_num_t pinNum = duk_get_int(ctx, -1);
	int level = gpio_get_level(pinNum);
	if (level == 0) {
		duk_push_false(ctx);
	} else {
		duk_push_true(ctx);
	}
	return 1;
} // js_os_gpioGetLevel
#endif // ESP_PLATFORM


/**
 * Listen on a server socket.
 * [0] - Params object
 * - sockfd: The socket to bind.
 */
static duk_ret_t js_os_listen(duk_context *ctx) {
	int sockfd;
	LOGD(">> js_os_listen");
	if (!duk_is_object(ctx, -1)) {
		LOGE("js_os_listen: No parameters object found.");
		return 0;
	}
	if (!duk_get_prop_string(ctx, -1, "sockfd")) {
		LOGE("js_os_listen: No sockfd property found.");
		return 0;
	}
	if (!duk_is_number(ctx, -1)) {
		LOGE("js_os_listen: Sockfd property is not a number.");
		return 0;
	}

	sockfd = duk_get_int(ctx, -1);
	duk_pop(ctx);

	LOGD("About to call listen on fd=%d", sockfd);
	int rc = listen(sockfd, 5);
	if (rc != 0) {
		LOGE("Error with listen: %d %d %s", rc, errno, strerror(errno));
	}
	duk_push_int(ctx, rc);
	LOGD("<< js_os_listen");
	return 1;
} // js_os_listen


/**
 * Receive data from the socket.
 * The input is a parameters object that contains:
 * - sockfd - The socket we are to read from.
 * - data - A buffer used to hold the received data.
 *
 * The return is the amount of data actually received.
 */
static duk_ret_t js_os_recv(duk_context *ctx) {
	duk_size_t size;
	ssize_t recvRc;
	LOGD(">> js_os_recv");
	void *data;
	if (!duk_is_object(ctx, -1)) {
		LOGE("js_os_recv: No parameters object found.");
		return 0;
	}

	if (!duk_get_prop_string(ctx, -1, "sockfd")) {
		LOGE("js_os_recv: No sockfd property found.");
		return 0;
	}
	int sockfd = duk_get_int(ctx, -1);
	duk_pop(ctx);

	if (!duk_get_prop_string(ctx, -1, "data")) {
		LOGE("js_os_recv: No data property found.");
		return 0;
	}
	/*
	if (!duk_is_buffer(ctx, -1)) {
		ESP_LOGE(tag, "js_os_recv: The data property is not a buffer.");
		return 0;
	}
	*/

	data = duk_get_buffer_data(ctx, -1, &size);
	if (size == 0) {
		LOGE("js_os_recv: The data buffer is zero length.");
		return 0;
	}
	LOGD("-- js_os_recv: About to receive on fd=%d for a buffer of size %d", sockfd, (int)size);
	recvRc = recv(sockfd, data, size, 0);
	if (recvRc < 0) {
		LOGE("Error with recv: %d: %d - %s", (int)recvRc, errno, strerror(errno));
		recvRc=0;
	}
	duk_push_int(ctx, recvRc);
	LOGD("<< js_os_recv: length=%d", (int)recvRc);
	return 1;
} // js_os_recv


/**
 * Select from an array of sockets.
 * [0] - Parms object
 *  - readfds - read socket array
 *  - writefds - write socket array
 *  - exceptfds - exception socket array
 *  Logging is Verbose as it is heavily trafficed.
 */
static duk_ret_t js_os_select(duk_context *ctx) {
	duk_size_t arraySize;
	fd_set readfds;
	fd_set writefds;
	fd_set exceptfds;
	int *readfdsArray;
	int *writefdsArray;
	int *exceptfdsArray;
	int readfdsCount, writefdsCount, exceptfdsCount;
	int max = -1;
	int i, idx;

	//LOGV(">> js_os_select");
	if (!duk_is_object(ctx, -1)) {
		LOGE("js_os_select: No parameters object found.");
		return 0;
	}

	FD_ZERO(&readfds);
	FD_ZERO(&writefds);
	FD_ZERO(&exceptfds);
	readfdsArray = writefdsArray = exceptfdsArray = NULL;
	readfdsCount = writefdsCount = exceptfdsCount = 0;

	if (duk_get_prop_string(ctx, -1, "readfds")) {
		// [0] - Parms object
		// [1] - readfds array

		if (duk_is_array(ctx, -1)) {
			readfdsCount = arraySize = duk_get_length(ctx, -1);
			readfdsArray = calloc(arraySize, sizeof(int));
			for (i=0; i<arraySize; i++) {
				duk_get_prop_index(ctx, -1, i);
				// [0] - Parms object
				// [1] - readfds array
				// [2] - fds val (i)
				int val = duk_get_int(ctx, -1);
				if (val > max) {
					max = val;
				}
				FD_SET(val, &readfds);
				readfdsArray[i] = val;
				duk_pop(ctx);
				// [0] - Parms object
				// [1] - readfds array
			}
			duk_pop(ctx);
			// [0] - Parms object
		} // End of is an array
	} // End of if readfs


	if (duk_get_prop_string(ctx, -1, "writefds")) {
		// [0] - Parms object
		// [1] - writefds array

		if (duk_is_array(ctx, -1)) {
			writefdsCount = arraySize = duk_get_length(ctx, -1);
			writefdsArray = calloc(arraySize, sizeof(int));
			for (i=0; i<arraySize; i++) {
				duk_get_prop_index(ctx, -1, i);
				// [0] - Parms object
				// [1] - writefds array
				// [2] - fds val (i)
				int val = duk_get_int(ctx, -1);
				if (val > max) {
					max = val;
				}
				FD_SET(val, &writefds);
				writefdsArray[i] = val;
				duk_pop(ctx);
				// [0] - Parms object
				// [1] - writefds array
			}
			duk_pop(ctx);
			// [0] - Parms object
		}
	}

	if (duk_get_prop_string(ctx, -1, "exceptfds")) {
		// [0] - Parms object
		// [1] - exceptfds array

		if (duk_is_array(ctx, -1)) {
			exceptfdsCount = arraySize = duk_get_length(ctx, -1);
			exceptfdsArray = calloc(arraySize, sizeof(int));

			for (i=0; i<arraySize; i++) {
				duk_get_prop_index(ctx, -1, i);
				// [0] - Parms object
				// [1] - exceptfds array
				// [2] - fds val (i)
				int val = duk_get_int(ctx, -1);
				if (val > max) {
					max = val;
				}
				FD_SET(val, &exceptfds);
				exceptfdsArray[i] = val;
				duk_pop(ctx);
				// [0] - Parms object
				// [1] - exceptfds array
			}
			duk_pop(ctx);
			// [0] - Parms object
		}
	}

	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	//LOGV(" - select(bitsToScan=%d, count=%d, count=%d, count=%d...)", max+1, readfdsCount, writefdsCount, exceptfdsCount);
	//ESP_LOGV(tag, " - readfds: 0x%x", (int)(readfds.fds_bits[0]));
	int rc = select(max+1, &readfds, &writefds, &exceptfds, &tv);
	if (rc < 0) {
		LOGE("Error with select: %d: %d - %s", rc, errno, strerror(errno));
		return 0;
	}
	/*
#ifdef ESP_PLATFORM
	LOGV("- rc from select = %d - readfds=0x%x, writefds=0x%x, exceptfds=0x%x", rc,
			(int)(readfds.fds_bits[0]), (int)(writefds.fds_bits[0]), (int)(exceptfds.fds_bits[0]));
#endif
*/

	/**
	 * Build the return which is an object that contains:
	 * readfs: An array of fds that are ready to be read.
	 * writefds: An array of fds that are ready to be written.
	 * exceptfds: An array of fds that are in exception state.
	 */
	duk_push_object(ctx);

	duk_push_array(ctx);
	idx = 0;
	for (i=0; i<readfdsCount; i++) {
		if (FD_ISSET(readfdsArray[i], &readfds)) {
			duk_push_int(ctx, readfdsArray[i]);
			duk_put_prop_index(ctx, -2, idx);
			idx++;
		}
	}
	duk_put_prop_string(ctx, -2, "readfds");

	duk_push_array(ctx);
	idx = 0;
	for (i=0; i<writefdsCount; i++) {
		if (FD_ISSET(writefdsArray[i], &writefds)) {
			duk_push_int(ctx, writefdsArray[i]);
			duk_put_prop_index(ctx, -2, idx);
			idx++;
		}
	}
	duk_put_prop_string(ctx, -2, "writefds");

	duk_push_array(ctx);
	idx = 0;
	for (i=0; i<exceptfdsCount; i++) {
		if (FD_ISSET(exceptfdsArray[i], &exceptfds)) {
			duk_push_int(ctx, exceptfdsArray[i]);
			duk_put_prop_index(ctx, -2, idx);
			idx++;
		}
	}
	duk_put_prop_string(ctx, -2, "exceptfds");

	if (readfdsArray) {
		free(readfdsArray);
	}
	if (writefdsArray) {
		free(writefdsArray);
	}
	if (exceptfdsArray) {
		free(exceptfdsArray);
	}

	//LOGV("<< js_os_select");
	return 1;
} // js_os_select


/**
 * Send data to a partner socket.  The input to this function is a
 * parameter object that contains:
 * - sockfd - The socket file descriptor we will use to send data.
 * - data - A buffer or string that contains the data we wish to send.
 *
 * The return is the return code from the underlying OS send().
 */
static duk_ret_t js_os_send(duk_context *ctx) {
	ssize_t sendRc;
	duk_size_t size;
	void *data;
	int sockfd;

	LOGD(">> js_os_send");
	if (!duk_is_object(ctx, -1)) {
		LOGE("js_os_send: No parameters object found.");
		return 0;
	}

	if (!duk_get_prop_string(ctx, -1, "sockfd")) {
		LOGE("js_os_send: No sockfd property found.");
		return 0;
	}
	sockfd = duk_get_int(ctx, -1);
	duk_pop(ctx);

	if (!duk_get_prop_string(ctx, -1, "data")) {
		LOGE("js_os_send: No data property found.");
		return 0;
	}

	// If the data is a string, then the string is the data to transmit else it
	// is a buffer and the content of the buffer is the data to transmit,
	if (duk_is_string(ctx, -1)) {
		data = (void *)duk_get_string(ctx, -1);
		size = strlen(data);
	} else {
		data = duk_get_buffer_data(ctx, -1, &size);
		if (size == 0) {
			LOGE("js_os_send: The data buffer is zero length.");
			return 0;
		}
	}

	LOGD("About to send %d bytes of data to sockfd=%d", (int)size, sockfd);
	LOGD("- data: \"%.*s\"", (int)size, (char *)data);
	sendRc = send(sockfd, data, size, 0);

	if (sendRc < 0) {
		LOGE("Error with send: %d: %d - %s", (int)sendRc, errno, strerror(errno));
	}
	duk_push_int(ctx, sendRc);
	LOGD("<< js_os_send");
	return 1;
} // js_os_send


/**
 * Create a SHA1 encoding of data.
 * [0] - A string or buffer
 *
 * On return
 * A buffer (20 bytes long) containing the message digest.
 */
static duk_ret_t js_os_sha1(duk_context *ctx) {
	uint8_t *data;
	size_t length;

	// The input may be a string or a buffer so handle appropriately based on the type of input.
	if (duk_is_string(ctx, -1)) {
		data = (uint8_t *)duk_get_string(ctx, -1);
		length = strlen((char *)data);
	} else {
		data = duk_get_buffer_data(ctx, -1, &length);
	}

	if (data == NULL) {
		duk_push_null(ctx);
	}
	else {

		// We now have the data from which we wish to create the message digest.
		// We push a buffer of 20 bytes onto the value stack and get the handle to it.
		// This is where we will write the message digest into.
		unsigned char *result = duk_push_fixed_buffer(ctx, 20);

#if defined(ESP_PLATFORM)
		mbedtls_sha1(data, length, result);
#else /* ESP_PLATFORM */
		// The SHA1 function is part of openssl installed through libssl-dev.  One must link
		// with -lcrypto.
		SHA1(data, length, result);
#endif /* ESP_PLATFORM */

		// Convert the fixed buffer into a NodeJS Buffer object
		duk_push_buffer_object(ctx, -1, 0, 20, DUK_BUFOBJ_NODEJS_BUFFER);
	}
	return 1;
} // js_os_sha1


/**
 * Create a new socket.
 * The is no input to this function.
 *
 * The return is an object that contains:
 * * sockfd - The socket file descriptor.
 */
static duk_ret_t js_os_socket(duk_context *ctx) {
	LOGD(">> js_os_socket");
	int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sockfd < 0) {
		LOGE("Error with socket: %d: %d - %s", sockfd, errno, strerror(errno));
	} else {
		LOGD("New socket fd=%d", sockfd);
	}
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int));
	//fcntl(s, F_SETFL, fcntl(s, F_GETFL, 0) | O_NONBLOCK); // Set the socket to be non blocking.
	duk_push_object(ctx);
	duk_push_int(ctx, sockfd);
	duk_put_prop_string(ctx, -2, "sockfd");
	LOGD("<< js_os_socket");
	return 1;
} // js_os_socket



/**
 * Create the OS module in Global.
 */
void ModuleOS(duk_context *ctx) {
	LOGD("Registering moduleOS");
	duk_push_global_object(ctx);
	// [0] - Global object

	duk_idx_t idx = duk_push_object(ctx); // Create new OS object
	// [0] - Global object
	// [1] - New object - OS object

	duk_push_c_function(ctx, js_os_accept, 1);
	// [0] - Global object
	// [1] - New object - OS object
	// [2] - C Function - js_os_accept

	duk_put_prop_string(ctx, idx, "accept"); // Add accept to new OS
	// [0] - Global object
	// [1] - New object - OS object

	duk_push_c_function(ctx, js_os_bind, 1);
	// [0] - Global object
	// [1] - New object - OS object
	// [2] - C Function - js_os_bind

	duk_put_prop_string(ctx, idx, "bind"); // Add bind to new OS
	// [0] - Global object
	// [1] - New object - OS object

	duk_push_c_function(ctx, js_os_close, 1);
	// [0] - Global object
	// [1] - New object - OS object
	// [2] - C Function - js_os_close

	duk_put_prop_string(ctx, idx, "close"); // Add close to new OS
	// [0] - Global object
	// [1] - New object - OS object

	duk_push_c_function(ctx, js_os_closesocket, 1);
	// [0] - Global object
	// [1] - New object - OS object
	// [2] - C Function - js_os_closesocket

	duk_put_prop_string(ctx, idx, "closesocket"); // Add closesocket to new OS
	// [0] - Global object
	// [1] - New object - OS object

	duk_push_c_function(ctx, js_os_connect, 1);
	// [0] - Global object
	// [1] - New object - OS object
	// [2] - C Function - js_os_connect

	duk_put_prop_string(ctx, idx, "connect"); // Add connect to new OS
	// [0] - Global object
	// [1] - New object - OS object

	duk_push_c_function(ctx, js_os_gethostbyname, 1);
	// [0] - Global object
	// [1] - New object - OS object
	// [2] - C Function - js_os_gethostbyname

	duk_put_prop_string(ctx, idx, "gethostbyname"); // Add gethostbyname to new OS
	// [0] - Global object
	// [1] - New object - OS object


#if defined(ESP_PLATFORM)
	duk_push_c_function(ctx, js_os_gpioGetLevel, 1);
	// [0] - Global object
	// [1] - New object - OS object
	// [2] - C Function - js_os_gpioGetLevel

	duk_put_prop_string(ctx, idx, "gpioGetLevel"); // Add gpioGetLevel to new OS
	// [0] - Global object
	// [1] - New object - OS object

	duk_push_c_function(ctx, js_os_gpioInit, 1);
	// [0] - Global object
	// [1] - New object - OS object
	// [2] - C Function - js_os_gpioInit

	duk_put_prop_string(ctx, idx, "gpioInit"); // Add js_os_gpioInit to new OS
	// [0] - Global object
	// [1] - New object - OS object

	duk_push_c_function(ctx, js_os_gpioSetDirection, 2);
	// [0] - Global object
	// [1] - New object - OS object
	// [2] - C Function - js_os_setDirection

	duk_put_prop_string(ctx, idx, "gpioSetDirection"); // Add gpioSetDirection to new OS
	// [0] - Global object
	// [1] - New object - OS object

	duk_push_c_function(ctx, js_os_gpioSetLevel, 2);
	// [0] - Global object
	// [1] - New object - OS object
	// [2] - C Function - js_os_gpioSetLevel

	duk_put_prop_string(ctx, idx, "gpioSetLevel"); // Add gpioSetLevel to new OS
	// [0] - Global object
	// [1] - New object - OS object
#endif // ESP_PLATFORM

	duk_push_c_function(ctx, js_os_listen, 1);
	// [0] - Global object
	// [1] - New object - OS object
	// [2] - C Function - js_os_listen

	duk_put_prop_string(ctx, idx, "listen"); // Add listen to new OS
	// [0] - Global object
	// [1] - New object - OS object

	duk_push_c_function(ctx, js_os_recv, 1);
	// [0] - Global object
	// [1] - New object - OS object
	// [2] - C Function - js_os_recv

	duk_put_prop_string(ctx, idx, "recv"); // Add recv to new OS
	// [0] - Global object
	// [1] - New object - OS object

	duk_push_c_function(ctx, js_os_select, 1);
	// [0] - Global object
	// [1] - New object - OS object
	// [2] - C Function - js_os_select

	duk_put_prop_string(ctx, idx, "select"); // Add select to new OS
	// [0] - Global object
	// [1] - New object - OS object

	duk_push_c_function(ctx, js_os_send, 1);
	// [0] - Global object
	// [1] - New object - OS object
	// [2] - C Function - js_os_send

	duk_put_prop_string(ctx, idx, "send"); // Add send to new OS
	// [0] - Global object
	// [1] - New object - OS object

	duk_push_c_function(ctx, js_os_sha1, 1);
	// [0] - Global object
	// [1] - New object - OS object
	// [2] - C Function - js_os_sha1

	duk_put_prop_string(ctx, idx, "sha1"); // Add sha1 to new OS
	// [0] - Global object
	// [1] - New object - OS object

	duk_push_c_function(ctx, js_os_socket, 0);
	// [0] - Global object
	// [1] - New object - OS object
	// [2] - C Function - js_os_socket

	duk_put_prop_string(ctx, idx, "socket"); // Add socket to new OS
	// [0] - Global object
	// [1] - New object - OS object


	duk_put_prop_string(ctx, 0, "OS"); // Add OS to global
	// [0] - Global object

	duk_pop(ctx);
	// <Empty Stack>
} // ModuleOS
