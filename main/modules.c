#if defined(ESP_PLATFORM)
#include <esp_system.h>
#include <espfs.h>

#include "esp32_specific.h"
#include "sdkconfig.h"
#endif // ESP_PLATFORM

#include <assert.h>
#include <duktape.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>

#include "duk_trans_socket.h" // The debug functions from Duktape.
#include "duktape_utils.h"
#include "logging.h"
#include "modules.h"
#include "module_bluetooth.h"
#include "module_dukf.h"
#include "module_fs.h"
#include "module_ledc.h"
#include "module_nvs.h"
#include "module_os.h"
#include "module_partitions.h"
#include "module_rmt.h"
#include "module_rtos.h"
#include "module_serial.h"
#include "module_ssl.h"
#include "module_timers.h"
#include "module_wifi.h"

LOG_TAG("modules");

/**
 * The native Console.log() static function.  This function is
 * also cognizant of the console.handler callback function
 */
static duk_ret_t js_console_log(duk_context *ctx) {
	LOGD(">> js_console_log called");
	duk_safe_to_string(ctx, -1);
	const char *message = duk_get_string(ctx, -1);

	// Let us now see if there is a console callback.  It will be
	// console.handler = function(message)
	duk_push_global_object(ctx);
	duk_get_prop_string(ctx, -1, "console");
	duk_get_prop_string(ctx, -1, "handler");
	if (duk_is_function(ctx, -1)) {
		// Aha ... there is a console.handler callback!
		// Let us now call it.
		duk_push_string(ctx, message);
		duk_pcall(ctx, 1);
	} // Check for a handler
	else {
		// No handler (either doesn't exist or not a function ... so just log
		esp32_duktape_console(message);
	}
  return 0;
} // js_console_log


typedef struct {
	char *id;
	duk_c_function func;
	int paramCount;
} functionTableEntry_t;


/*
 * Table of functions to be mapped to strings.  The format of an entry is
 * { <Name>, <FunctionPtr>, <Parameter Count> },
 *
 * The last entry must be:
 * { NULL, NULL, 0 }
 */
functionTableEntry_t functionTable[] = {
#if defined(ESP_PLATFORM)
#if defined(ESP32_DUKTAPE_USE_BLUETOOTH)
	{ "ModuleBluetooth",  ModuleBluetooth,  1},
#endif
	{ "ModuleLEDC",       ModuleLEDC,       1},
	{ "ModulePartitions", ModulePartitions, 1},
	{ "ModuleRMT",        ModuleRMT,        1},
	{ "ModuleRTOS",       ModuleRTOS,       1},
	{ "ModuleSerial",     ModuleSerial,     1},
	{ "ModuleSSL",        ModuleSSL,        1},
#endif // ESP_PLATFORM
	// Must be last entry
	{NULL, NULL, 0 }
};


/**
 * Retrieve a native function reference by name.
 * ESP32.getNativeFunction(nativeFunctionID)
 * The input stack contains:
 * [ 0] - String - nativeFunctionID - A string name that is used to lookup a function handle.
 */
static duk_ret_t js_esp32_getNativeFunction(duk_context *ctx) {
	LOGD(">> js_esp32_getNativeFunction");
	// Check that the first parameter is a string.
	if (duk_is_string(ctx, 0)) {
		const char *nativeFunctionID = duk_get_string(ctx, 0);
		LOGD("- nativeFunctionId that we are looking for is \"%s\"", nativeFunctionID);

		// Lookup the handler function in a table.
		functionTableEntry_t *ptr = functionTable;
		while (ptr->id != NULL) {
			if (strcmp(nativeFunctionID, ptr->id) == 0) {
				break;
			}
			ptr++; // Not found yet, let's move to the next entry
		} // while we still have entries in the table

		// If we found an entry, then set it as the return, otherwise return null.
		if (ptr->id != NULL) {
			duk_push_c_function(ctx, ptr->func, ptr->paramCount);
		} else {
			LOGD("No native found found called %s", nativeFunctionID);
			duk_push_null(ctx);
		}
	} else {
		LOGD("No native function id supplied");
		duk_push_null(ctx);
	}
	// We will have either pushed null or a function reference onto the stack.
	LOGD("<< js_esp32_getNativeFunction");
	return 1;
} // js_esp32_getNativeFunction

#if defined(ESP_PLATFORM)
typedef struct {
	char *levelString;
	esp_log_level_t level;
} level_t;
static level_t levels[] = {
		{"none", ESP_LOG_NONE},
		{"error", ESP_LOG_ERROR},
		{"warn", ESP_LOG_WARN},
		{"info", ESP_LOG_INFO},
		{"debug", ESP_LOG_DEBUG},
		{"verbose", ESP_LOG_VERBOSE},
		{NULL, 0}
};
/**
 * Set the debug log level.
 * [0] - tag - The tag that we are setting the log level on.  Can be "*" for
 *             all tags.
 * [1] - level - The level we are setting for this tage.  Choices are:
 *               * none
 *               * error
 *               * warn
 *               * info
 *               * debug
 *               * verbose
 */
static duk_ret_t js_esp32_setLogLevel(duk_context *ctx) {
	char *tagToChange;
	char *levelString;
	tagToChange = (char *)duk_get_string(ctx, -2);
	levelString = (char *)duk_get_string(ctx, -1);
	LOGD("Setting a new log level to be tag: \"%s\", level: \"%s\"", tagToChange, levelString);
	level_t *pLevels = levels;
	while(pLevels->levelString != NULL) {
		if (strcmp(pLevels->levelString, levelString) == 0) {
			break;
		}
		pLevels++;
	}
	if (pLevels->levelString != NULL) {
		esp_log_level_set(tagToChange, pLevels->level);
	}
	return 0;
} // js_esp32_setLogLevel
#else /* ESP_PLATFORM */
static duk_ret_t js_esp32_setLogLevel(duk_context *ctx) {
	LOGD("set Log Level not implemented");
	return 0;
}
#endif /* ESP_PLATFORM */

/**
 * Load a file using the POSIX file I/O functions.
 * [0] - path - The name of the file to load.
 *
 * The String representation of the file will remain on the stack at the end.
 *
 * The high level algorithm is:
 * 1. Determine file size.
 * 2. Allocate a buffer to hold the file.
 * 3. Open the file.
 * 4. Read the whole file into the buffer.
 * 5. Cleanup.
 * 6. Put the buffer as a string onto the stack for return.
 *
 */
/*
static duk_ret_t js_esp32_loadFile(duk_context *ctx) {
	const char *path = duk_get_string(ctx, -1);
	struct stat statBuf;
	int rc = stat(path, &statBuf);
	if (rc < 0) {
		ESP_LOGD(tag, "js_esp32_loadFile: stat() %d %s", errno, strerror(errno));
		duk_push_null(ctx);
		return 1;
	}
	int fd = open(path, O_RDWR);
	if (fd < 0) {
		ESP_LOGD(tag, "js_esp32_loadFile: open() %d %s", errno, strerror(errno));
		duk_push_null(ctx);
		return 1;
	}
	char *data = malloc(statBuf.st_size);
	ssize_t sizeRead = read(fd, data, statBuf.st_size);
	if (sizeRead < 0) {
		ESP_LOGD(tag, "js_esp32_loadFile: read() %d %s", errno, strerror(errno));
		free(data);
		close(fd);
		duk_push_null(ctx);
		return 1;
	}
	close(fd); // Close the open file, we don't need it anymore.
	duk_push_lstring(ctx, data, sizeRead); // Push the data onto the stack.
	free(data); // Release the dynamically read data as it is on the stack now.
	ESP_LOGD(tag, "Read file %s of length %d", path, sizeRead);
	return 1;
} // js_esp32_loadFile
*/

static duk_ret_t js_esp32_loadFile(duk_context *ctx) {

	const char *path = duk_get_string(ctx, -1);
	LOGD(">> js_esp32_loadFile: %s", path);
	struct stat statBuf;
	int rc = stat(path, &statBuf);
	if (rc < 0) {
		LOGD("js_esp32_loadFile: stat() %d %s", errno, strerror(errno));
		duk_push_null(ctx);
		return 1;
	}
	int fd = open(path, O_RDWR);
	if (fd < 0) {
		LOGD("js_esp32_loadFile: open() %d %s", errno, strerror(errno));
		duk_push_null(ctx);
		return 1;
	}
	#define UNIT_SIZE (512)
	int actuallyRead = 0;
	int counter = 0;
	char *data = malloc(UNIT_SIZE);
	while (actuallyRead < statBuf.st_size) {
		ssize_t sizeRead = read(fd, data, UNIT_SIZE);
		if (sizeRead <= 0) {
			LOGD("js_esp32_loadFile: read() %d %s", errno, strerror(errno));
			free(data);
			close(fd);
			duk_push_null(ctx);
			return 1;
		}
		duk_push_lstring(ctx, data, sizeRead);
		counter++;
		actuallyRead += sizeRead;
	}
	duk_concat(ctx, counter);
	close(fd); // Close the open file, we don't need it anymore.
	free(data); // Release the dynamically read data as it is on the stack now.
	LOGD("<< js_esp32_loadFile: Read file %s of length %d", path, actuallyRead);
	return 1;
} // js_esp32_loadFile

#if defined(ESP_PLATFORM)
/**
 * Load a text file and push it onto the stack using the ESPFS technologies.
 */
static duk_ret_t js_esp32_loadFileESPFS(duk_context *ctx) {
	const char *path = duk_get_string(ctx, -1);
	LOGD(">> js_esp32_loadFileESPFS: %s", path);
	size_t fileSize;
	const char *fileText = esp32_loadFileESPFS(path, &fileSize);
	if (fileText == NULL) {
		LOGD(" Failed to open file %s", path);
		duk_push_null(ctx);
		return 1;
	}
  duk_push_lstring(ctx, fileText, fileSize);
  return 1;
} // js_esp32_loadFileESPFS


static duk_ret_t js_esp32_dumpESPFS(duk_context *ctx) {
	espFsDumpFiles();
	return 0;
} // js_esp32_dumpESPFS

/*
 * Reboot the ESP32.
 */
static duk_ret_t js_esp32_reboot(duk_context *ctx) {
	esp_restart();
	return 0;
} // js_esp32_reboot


#endif /* ESP_PLATFORM */




/**
 * Reset the duktape environment by flagging a request to reset.
 */
static duk_ret_t js_esp32_reset(duk_context *ctx) {
	esp32_duktape_set_reset(1);
	return 0;
} // js_esp32_reset

/**
 * ESP32.getState()
 * Return an object that describes the state of the ESP32 environment.
 * - heapSize - The available heap size.
 */
static duk_ret_t js_esp32_getState(duk_context *ctx) {

#if defined(ESP_PLATFORM)
	// [0] - New object
	duk_push_object(ctx); // Create new getState object

	// [0] - New object
	// [1] - heap size
	duk_push_number(ctx, (double)esp_get_free_heap_size());

	// [0] - New object
	duk_put_prop_string(ctx, -2, "heapSize"); // Add heapSize to new getState
#else /* ESP_PLATFORM */
	// [0] - New object
	duk_push_object(ctx); // Create new getState object

	// [0] - New object
	// [1] - heap size
	duk_push_number(ctx, (double)999999);

	// [0] - New object
	duk_put_prop_string(ctx, -2, "heapSize"); // Add heapSize to new getState
#endif /* ESP_PLATFORM */
	return 1;
} // js_esp32_getState



/**
 * Write a log record to the debug output stream.  Exposed
 * as the global log("message").
 */
static duk_ret_t js_global_log(duk_context *ctx) {
	LOGD_TAG("log", "%s", duk_safe_to_string(ctx, -1));
	return 0;
} // js_global_log


/**
 * Define the static module called "ModuleConsole".
 */
static void ModuleConsole(duk_context *ctx) {

	duk_push_global_object(ctx);
	// [0] Global Object

	duk_push_c_function(ctx, js_global_log, 1);
	// [0] Global Object
	// [1] C Function - js_global_log

	duk_put_prop_string(ctx, -2, "log"); // Add log to new console
	// [0] Global Object

	duk_push_object(ctx); // Create new console object
	// [0] Global Object
	// [1] New object

	duk_push_c_function(ctx, js_console_log, 1);
	// [0] Global Object
	// [1] New object
	// [2] c-function - js_console_log

	duk_put_prop_string(ctx, -2, "log"); // Add log to new console
	// [0] Global Object
	// [1] New object

	duk_put_prop_string(ctx, -2, "console"); // Add console to global
	// [0] Global Object

	duk_pop(ctx);
	// <stack empty>
} // ModuleConsole

#if defined(ESP_PLATFORM)
/**
 * Register the ESP32 module with its functions.
 */
static void ModuleESP32(duk_context *ctx) {
	duk_push_global_object(ctx);
	// [0] - Global object

	duk_push_object(ctx); // Create new ESP32 object
	// [0] - Global object
	// [1] - New object

	duk_push_c_function(ctx, js_esp32_dumpESPFS, 0);
	// [0] - Global object
	// [1] - New object
	// [2] - c-function - js_esp32_dumpESPFS

	duk_put_prop_string(ctx, -2, "dumpESPFS"); // Add dumpESPFS to new ESP32
	// [0] - Global object
	// [1] - New object

	duk_push_c_function(ctx, js_esp32_getNativeFunction, 1);
	// [0] - Global object
	// [1] - New object
	// [2] - c-function - js_esp32_getNativeFunction

	duk_put_prop_string(ctx, -2, "getNativeFunction"); // Add getNativeFunction to new ESP32
	// [0] - Global object
	// [1] - New object

	duk_push_c_function(ctx, js_esp32_getState, 0);
	// [0] - Global object
	// [1] - New object
	// [2] - c-function - js_esp32_getState

	duk_put_prop_string(ctx, -2, "getState"); // Add reset to new ESP32
	// [0] - Global object
	// [1] - New object

	duk_push_c_function(ctx, js_esp32_loadFile, 1);
	// [0] - Global object
	// [1] - New object
	// [2] - c-function - js_esp32_loadFile

	duk_put_prop_string(ctx, -2, "loadFile"); // Add loadFile to new ESP32
	// [0] - Global object
	// [1] - New object

	duk_push_c_function(ctx, js_esp32_loadFileESPFS, 1);
	// [0] - Global object
	// [1] - New object
	// [2] - c-function - js_esp32_loadFileESPFS

	duk_put_prop_string(ctx, -2, "loadFileESPFS"); // Add loadFileESPFS to new ESP32
	// [0] - Global object
	// [1] - New object


	duk_push_c_function(ctx, js_esp32_reboot, 0);
	// [0] - Global object
	// [1] - New object
	// [2] - c-function - js_esp32_reboot

	duk_put_prop_string(ctx, -2, "reboot"); // Add reboot to new ESP32
	// [0] - Global object
	// [1] - New object


	duk_push_c_function(ctx, js_esp32_reset, 0);
	// [0] - Global object
	// [1] - New object
	// [2] - c-function - js_esp32_reset

	duk_put_prop_string(ctx, -2, "reset"); // Add reset to new ESP32
	// [0] - Global object
	// [1] - New object

	duk_push_c_function(ctx, js_esp32_setLogLevel, 2);
	// [0] - Global object
	// [1] - New object
	// [2] - c-function - js_esp32_setLogLevel

	duk_put_prop_string(ctx, -2, "setLogLevel"); // Add setLogLevel to new ESP32
	// [0] - Global object
	// [1] - New object

	duk_put_prop_string(ctx, -2, "ESP32"); // Add ESP32 to global
	// [0] - Global object

	duk_pop(ctx);
	// <Empty stack>
} // ModuleESP32

#endif // ESP_PLATFORM

/**
 * Register the static modules.  These are modules that will ALWAYS
 * bein the global address space/scope.
 */
void registerModules(duk_context *ctx) {
#if defined(ESP_PLATFORM)
	espFsInit((void *)0x360000, 4 * 64 * 1024);
#endif // ESP_PLATFORM

	duk_idx_t top = duk_get_top(ctx);
	ModuleConsole(ctx);
	assert(top == duk_get_top(ctx));

	ModuleFS(ctx);
	assert(top == duk_get_top(ctx));

	ModuleOS(ctx);
	assert(top == duk_get_top(ctx));

	ModuleDUKF(ctx);
	assert(top == duk_get_top(ctx));

#if defined(ESP_PLATFORM)
	ModuleESP32(ctx);
	assert(top == duk_get_top(ctx));

	ModuleWIFI(ctx); // Load the WiFi module
	assert(top == duk_get_top(ctx));

	ModuleNVS(ctx); // Load the Non Volatile Storage module
	assert(top == duk_get_top(ctx));

	//ModuleTIMERS(ctx);
	//assert(top == duk_get_top(ctx));
#endif /* ESP_PLATFORM */

} // End of registerModules
