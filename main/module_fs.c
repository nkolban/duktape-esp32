#include <stdbool.h>
#include <esp_log.h>
#include <esp_system.h>
#include <duktape.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include "duktape_utils.h"
#include "sdkconfig.h"

static char tag[] = "module_fs";

static int stringToPosixFlags(char *flags) {
	int posixFlags = 0;
	while (*flags != '\0') {
		if (*flags == 'r') {
			posixFlags |= O_RDONLY;
		} else if (*flags == 'w') {
			posixFlags |= O_WRONLY;
		}
		flags++;
	}
	return posixFlags;
} // stringToPosixFlags

/**
 * The native fs.openSync
 * * path <String> | <Buffer>
 * * flags <String> | <Number>
 * * mode <Integer>
 *
 * The flags are "r", "w"
 */
static duk_ret_t js_fs_openSync(duk_context *ctx) {
	ESP_LOGD(tag, ">> js_fs_openSync");
	// Get the path
	const char *path = duk_get_string(ctx, 0);
	if (path == NULL) {
		ESP_LOGD(tag, "<< js_fs_openSync: Unable to get path");
		return DUK_RET_ERROR;
	}
	ESP_LOGD(tag, " - Path to open is %s", path);
	char *flags = "r";
	int posixOpenFlags = stringToPosixFlags(flags);
	int fd = open(path, posixOpenFlags);
	if (fd < 0) {
		ESP_LOGD(tag, "<< js_fs_openSync: Error open(): %d %s", errno, strerror(errno));
		return DUK_RET_ERROR;
	}
	duk_push_int(ctx, fd);
	ESP_LOGD(tag, "<< js_fs_openSync fd: %d", fd);
  return 1;
} // js_fs_openSync


static duk_ret_t js_fs_closeSync(duk_context *ctx) {
	int fd = duk_require_int(ctx, 0);
	close(fd);
	return 0;
} // js_fs_closeSync


/**
 * Read synchronously from a file.  The file to be read is specified by the
 * file descriptor which should previously have been opened.
 *
 * fd <Integer> - file descriptor - 0
 * buffer <Buffer> - The buffer into which to read data - 1
 * writeOffset <Integer> - The offset into the buffer to start writing - 2
 * maxToRead <Integer> - The maximum number of bytes to read - 3
 * position <Integer> - The position within the file to read from.  If position
 *    is null then we read from the current file position. - 4
 *
 * return:
 * Number of bytes read.
 */
static duk_ret_t js_fs_readSync(duk_context *ctx) {
	ESP_LOGD(tag, ">> js_fs_readSync");
	esp32_duktape_dump_value_stack(ctx);
	int fd = duk_require_int(ctx, 0);
	size_t writeOffset = duk_require_int(ctx, 2);
	size_t maxToRead = duk_require_int(ctx, 3);
	//size_t position = duk_require_int(ctx, 4);
	ssize_t sizeRead = 0;
	duk_size_t bufferSize;

	uint8_t *bufPtr = duk_require_buffer_data(ctx, 1, &bufferSize);
	if (bufPtr == NULL) {
		ESP_LOGD(tag, "Failed to get the buffer pointer");
	}
	ESP_LOGD(tag, "Buffer pointer size returned as %d", bufferSize);

	// Length can't be <= 0.
	// FIX ... PERFORM CHECK HERE

	// Check that writeOffset is within range.  If the buffer is "buffer.length" bytes in size
	// then the writeOffset must be < buffer.length
	if (writeOffset >= bufferSize) {
		ESP_LOGD(tag, "Invalid writeOffset");
	}

	// Position can't be < 0
	// FIX ... PERFORM CHECK HERE

	// if length > buffer.length -> length = buffer.length
	if (maxToRead > bufferSize) {
		maxToRead = bufferSize;
	}

	// Read the data from the underlying file.
	sizeRead = read(fd, bufPtr+writeOffset, maxToRead);
	if (sizeRead < 0) {
		ESP_LOGD(tag, "js_fs_readSync: read() error: %d %s", errno, strerror(errno));
		sizeRead = 0;
	}

	duk_push_int(ctx, sizeRead); // Push the size onto the value stack.
	ESP_LOGD(tag, "<< js_fs_readSync: sizeRead: %d", sizeRead);
	return 1;
} // js_fs_readSync


/**
 * Create the FS module in Global.
 */
void ModuleFS(duk_context *ctx) {
	duk_push_global_object(ctx);
	// [0] - Global

	duk_push_object(ctx); // Create new FS object
	// [0] - Global
	// [1] - FS Object

	duk_push_c_function(ctx, js_fs_openSync, 3);
	// [0] - Global
	// [1] - FS Object
	// [2] - C Func - js_fs_openSync

	duk_put_prop_string(ctx, -2, "openSync"); // Add openSync to new FS
	// [0] - Global
	// [1] - FS Object

	duk_push_c_function(ctx, js_fs_closeSync, 1);
	// [0] - Global
	// [1] - FS Object
	// [2] - C Func - js_fs_closeSync

	duk_put_prop_string(ctx, -2, "closeSync"); // Add closeSync to new FS
	// [0] - Global
	// [1] - FS Object

	duk_push_c_function(ctx, js_fs_readSync, 5);
	// [0] - Global
	// [1] - FS Object
	// [2] - C Func - js_fs_readSync

	duk_put_prop_string(ctx, -2, "readSync"); // Add readSync to new FS
	// [0] - Global
	// [1] - FS Object

	duk_put_prop_string(ctx, -2, "FS"); // Add ESP32 to global
	// [0] - Global

	duk_pop(ctx);
	// <empty stack>
} // ModuleFS
