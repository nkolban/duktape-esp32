/**
 * Create the FS Module which provides I/O operations.  These map
 * to Posix I/O operations.
 */
#if defined(ESP_PLATFORM)
#include <esp_log.h>
#include <esp_system.h>
#include "duktape_spiffs.h"
#include "sdkconfig.h"
#else // ESP_PLATFORM
#include <dirent.h>
#endif // ESP_PLATFORM

#include <duktape.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include "duktape_utils.h"
#include "logging.h"
#include "module_fs.h"

LOG_TAG("module_fs");

/**
 * Convert a string to posix open() flags.
 * "r" - O_RDONLY
 * "w" - O_WRONLY
 */
static int stringToPosixFlags(const char *flags) {
	int posixFlags = 0;
	if (strcmp(flags, "r") == 0) {
		posixFlags = O_RDONLY;
	}
	if (strcmp(flags, "r+") == 0) {
		posixFlags = O_RDWR;
	}
	if (strcmp(flags, "w") == 0) {
		posixFlags = O_WRONLY | O_TRUNC | O_CREAT;
	}
	if (strcmp(flags, "w+") == 0) {
		posixFlags = O_RDWR | O_TRUNC | O_CREAT;
	}
	if (strcmp(flags, "a") == 0) {
		posixFlags = O_WRONLY | O_APPEND | O_CREAT;
	}
	if (strcmp(flags, "a+") == 0) {
		posixFlags = O_RDWR | O_APPEND | O_CREAT;
	}
	return posixFlags;
} // stringToPosixFlags


/**
 * Write data to a file synchronously.
 *
 * [0] - file descriptor
 * [1] - buffer
 * [2] - offset [Optional]
 * [3] - length [Optional]
 *
 * or
 *
 * [0] - file descriptor
 * [1] - string
 *
 */
static duk_ret_t js_fs_writeSync(duk_context *ctx) {
	int fd;
	duk_size_t bufferSize;
	uint8_t *bufPtr;
	int rc;
	duk_size_t offset = 0; // Default offset is 0.
	duk_size_t length;

	fd = duk_get_int(ctx, 0);
	// [0] - file descriptor
	// [1] - buffer
	// [2] - offset [Optional]
	// [3] - length [Optional]

	if (duk_is_buffer(ctx, 1)) {
		bufPtr = duk_require_buffer_data(ctx, 1, &bufferSize);
		// [0] - file descriptor
		// [1] - buffer
		// [2] - offset [Optional]
		// [3] - length [Optional]

		length = bufferSize; // Unless specified, the length is all of the buffer

		if (duk_is_number(ctx, 2)) {
		// [0] - file descriptor
		// [1] - buffer
		// [2] - offset [Optional]
		// [3] - length [Optional]

			offset = duk_get_int(ctx, 2);
		// [0] - file descriptor
		// [1] - buffer
		// [2] - offset [Optional]
		// [3] - length [Optional]

			if (duk_is_number(ctx, 3)) {
		// [0] - file descriptor
		// [1] - buffer
		// [2] - offset [Optional]
		// [3] - length [Optional]

				length = duk_get_int(ctx, 3);
		// [0] - file descriptor
		// [1] - buffer
		// [2] - offset [Optional]
		// [3] - length [Optional]
				if (length > bufferSize) {
					LOGW("Specified length is > buffer size");
					length = bufferSize;
				} // length > bufferSize
			} // We have a length
		} // We have an offset
	} // Data is buffer
	else { // Handle the case where the data type is a string.
		bufPtr = (uint8_t *)duk_get_string(ctx, 1);
		// [0] - file descriptor
		// [1] - string

		length = bufferSize = strlen((char *)bufPtr);
	}
	if (length > (bufferSize - offset)) {
		LOGW("Specified length + offset is > buffer size");
		length = bufferSize - offset;
	}
	rc = write(fd, bufPtr+offset, length);

	duk_push_int(ctx, rc);
	// [0] - file descriptor
	// [1] - buffer
	// [2] - offset [Optional]
	// [3] - length [Optional]
	// [4] - write rc

	return 1;
} // js_fs_writeSync


/**
 * The native fs.openSync.  Open a file by name and return a file
 * descriptor (<numeric>).
 * * path <String> | <Buffer>
 * * flags <String> | <Number>
 * * mode <Integer>
 *
 * The flags are "r", "w"
 * [0] - Path
 * [1] - Flags
 */
static duk_ret_t js_fs_openSync(duk_context *ctx) {
	LOGD(">> js_fs_openSync");

	const char *path = duk_get_string(ctx, 0); 	// Get the path
	// [0] - Path
	// [1] - Flags

	if (path == NULL) {
		LOGD("<< js_fs_openSync: Unable to get path");
		return DUK_RET_ERROR;
	}

	const char *flags = duk_get_string(ctx, 1); // Get the flags
	// [0] - Path
	// [1] - Flags

	if (flags == NULL) {
		LOGD("<< js_fs_openSync: Unable to get flags");
		return DUK_RET_ERROR;
	}

	LOGD(" - Path to open is \"%s\" and flags are \"%s\"", path, flags);
	int posixOpenFlags = stringToPosixFlags(flags);

	int fd = open(path, posixOpenFlags, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
	if (fd < 0) {
		LOGD("<< js_fs_openSync: Error: open(%s, 0x%x): %d errno=%d [%s]", path, posixOpenFlags, fd, errno, strerror(errno));
		return DUK_RET_ERROR;
	}
	duk_push_int(ctx, fd); // Return the open file descriptor
	// [0] - Path <string>
	// [1] - File descriptor <numeric>
	LOGD("<< js_fs_openSync fd: %d", fd);

  return 1;
} // js_fs_openSync


static duk_ret_t js_fs_closeSync(duk_context *ctx) {
	int fd = duk_require_int(ctx, 0);
	close(fd);
	return 0;
} // js_fs_closeSync


static duk_ret_t js_fs_fstatSync(duk_context *ctx) {
	struct stat statBuf;
	int fd = duk_require_int(ctx, 0);
	int rc = fstat(fd, &statBuf);
	if (rc == -1) {
		LOGD("Error from stat of fd %d: %d %s", fd, errno, strerror(errno));
		return 0;
	}
	duk_push_object(ctx);
	duk_push_int(ctx, statBuf.st_size);
	duk_put_prop_string(ctx, -2, "size");
	return 1;
} // js_fs_fstatSync


/**
 * Determine the stats on the file.
 * [0] - Path name
 */
static duk_ret_t js_fs_statSync(duk_context *ctx) {
	struct stat statBuf;
	const char *path = duk_require_string(ctx, 0);
	int rc = stat(path, &statBuf);
	if (rc == -1) {
		LOGD("Error from stat of file %s: %d %s", path, errno, strerror(errno));
		return DUK_RET_ERROR;
	}
	duk_push_object(ctx);
	duk_push_int(ctx, statBuf.st_size);
	duk_put_prop_string(ctx, -2, "size");
	return 1;
} // js_fs_fstatSync

/**
 * Perform a stat on the file.  The return data includes:
 * * size - Number of bytes in the file.
 */
static duk_ret_t js_fs_dump(duk_context *ctx) {
#if defined(ESP_PLATFORM)
	esp32_duktape_dump_spiffs();
#endif // ESP_PLATFORM
	return 0;
}


/**
 * Read synchronously from a file.  The file to be read is specified by the
 * file descriptor which should previously have been opened.
 *
 * [0] - fd <Integer> - file descriptor - 0
 * [1] - buffer <Buffer> - The buffer into which to read data - 1
 * [2] - writeOffset <Integer> - The offset into the buffer to start writing - 2
 * [3] - maxToRead <Integer> - The maximum number of bytes to read - 3
 * [4] - position <Integer> - The position within the file to read from.  If position
 *    is null then we read from the current file position. - 4
 *
 * return:
 * Number of bytes read.
 */
static duk_ret_t js_fs_readSync(duk_context *ctx) {
	LOGD(">> js_fs_readSync");
	//esp32_duktape_dump_value_stack(ctx);
	int fd = duk_require_int(ctx, 0);
	// [0] - fd <Integer>
	// [1] - buffer <Buffer>
	// [2] - writeOffset <Integer>
	// [3] - maxToRead <Integer>
	// [4] - position <Integer>

	size_t writeOffset = duk_require_int(ctx, 2);
	// [0] - fd <Integer>
	// [1] - buffer <Buffer>
	// [2] - writeOffset <Integer>
	// [3] - maxToRead <Integer>
	// [4] - position <Integer>

	size_t maxToRead = duk_require_int(ctx, 3);
	// [0] - fd <Integer>
	// [1] - buffer <Buffer>
	// [2] - writeOffset <Integer>
	// [3] - maxToRead <Integer>
	// [4] - position <Integer>

	//size_t position = duk_require_int(ctx, 4);
	ssize_t sizeRead = 0;
	duk_size_t bufferSize;

	uint8_t *bufPtr = duk_require_buffer_data(ctx, 1, &bufferSize);
	// [0] - fd <Integer>
	// [1] - buffer <Buffer>
	// [2] - writeOffset <Integer>
	// [3] - maxToRead <Integer>
	// [4] - position <Integer>

	if (bufPtr == NULL) {
		LOGD("Failed to get the buffer pointer");
	}
	LOGD("Buffer pointer size returned as %d", (int)bufferSize);

	// Length can't be <= 0.
	// FIX ... PERFORM CHECK HERE

	// Check that writeOffset is within range.  If the buffer is "buffer.length" bytes in size
	// then the writeOffset must be < buffer.length
	if (writeOffset >= bufferSize) {
		LOGD("Invalid writeOffset");
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
		LOGD("js_fs_readSync: read() error: %d %s", errno, strerror(errno));
		sizeRead = 0;
	}

	duk_push_int(ctx, sizeRead); // Push the size onto the value stack.
	// [0] - fd <Integer>
	// [1] - buffer <Buffer>
	// [2] - writeOffset <Integer>
	// [3] - maxToRead <Integer>
	// [4] - position <Integer>
	// [5] - size read <Integer>

	LOGD("<< js_fs_readSync: sizeRead: %d", (int)sizeRead);
	return 1;
} // js_fs_readSync

/**
 * Unlink the named file.
 * [0] - Path.  The path to the file to unlink.
 */
static duk_ret_t js_fs_unlink(duk_context *ctx) {
	const char *path = duk_require_string(ctx, 0);
	unlink(path);
	return 0;
} // js_fs_unlink


/**
 * Get a listing of SPIFFs files.
 * The return is a JS array which contains objects.  Each
 * object represents one file entry.  Each object contains:
 * {
 *    name: <file name>
 *    size: <file size>
 * }
 */
static duk_ret_t js_fs_spiffsDir(duk_context *ctx) {
#if defined(ESP_PLATFORM)
	esp32_duktape_dump_spiffs_array(ctx);
	return 1;

#else // ESP_PLATFORM
	duk_push_array(ctx); // Push a new empty array onto the stack.
	DIR *d;
	struct dirent *dir;
	duk_push_global_object(ctx);
	duk_get_prop_string(ctx, -1, "DUKF");
	duk_get_prop_string(ctx, -1, "FILE_SYSTEM_ROOT");
	const char *path = duk_get_string(ctx, -1);
	if (path == NULL) {
		LOGE("Unable to get DUKF.FILE_SYSTEM_ROOT");
		return 0;
	}
	d = opendir(path);
	duk_pop_3(ctx);
	if (d != NULL) {
		int i=0;
		while((dir = readdir(d)) != NULL) {
			// The dir object now points to a dirent.

			// Skip if "." or ".."
			if (strcmp(dir->d_name, ".") == 0 ||
					strcmp(dir->d_name, "..") ==0) {
				continue;
			}
			duk_push_object(ctx);
			duk_push_string(ctx, dir->d_name);
			duk_put_prop_string(ctx, -2, "name");
			duk_push_int(ctx, 0);
			duk_put_prop_string(ctx, -2, "size");
			duk_put_prop_index(ctx, -2, i);
			i++;
		}
		closedir(d);
	}
	return 1;
#endif  // ESP_PLATFORM
} // js_fs_spiffsDir


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

	duk_push_c_function(ctx, js_fs_writeSync, 4);
	// [0] - Global
	// [1] - FS Object
	// [2] - C Func - js_fs_writeSync

	duk_put_prop_string(ctx, -2, "writeSync"); // Add writeSync to new FS
	// [0] - Global
	// [1] - FS Object

	duk_push_c_function(ctx, js_fs_statSync, 1);
	// [0] - Global
	// [1] - FS Object
	// [2] - C Func - js_fs_statSync

	duk_put_prop_string(ctx, -2, "statSync"); // Add statSync to new FS
	// [0] - Global
	// [1] - FS Object

	duk_push_c_function(ctx, js_fs_fstatSync, 1);
	// [0] - Global
	// [1] - FS Object
	// [2] - C Func - js_fs_fstatSync

	duk_put_prop_string(ctx, -2, "fstatSync"); // Add fstatSync to new FS
	// [0] - Global
	// [1] - FS Object

	duk_push_c_function(ctx, js_fs_unlink, 1);
	// [0] - Global
	// [1] - FS Object
	// [2] - C Func - js_fs_unlink

	duk_put_prop_string(ctx, -2, "unlink"); // Add unlink to new FS
	// [0] - Global
	// [1] - FS Object

	duk_push_c_function(ctx, js_fs_dump, 0);
	// [0] - Global
	// [1] - FS Object
	// [2] - C Func - js_fs_dump

	duk_put_prop_string(ctx, -2, "dump"); // Add dump to new FS
	// [0] - Global
	// [1] - FS Object

	duk_push_c_function(ctx, js_fs_spiffsDir, 0);
	// [0] - Global
	// [1] - FS Object
	// [2] - C Func - js_fs_spiffsDir

	duk_put_prop_string(ctx, -2, "spiffsDir"); // Add spiffsDir to new FS
	// [0] - Global
	// [1] - FS Object

	duk_put_prop_string(ctx, -2, "FS"); // Add FS to global
	// [0] - Global

	duk_pop(ctx);
	// <empty stack>
} // ModuleFS
