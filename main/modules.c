#include <stdbool.h>
#include <esp_log.h>
#include <esp_system.h>
#include <duktape.h>
#include <stdlib.h>
#include <assert.h>
#include "modules.h"
#include "esp32_duktape/curl_client.h"
#include "esp32_duktape/module_fs.h"
#include "esp32_duktape/module_gpio.h"
#include "esp32_duktape/module_timers.h"
#include "esp32_duktape/module_rmt.h"
#include "esp32_duktape/module_http.h"
#include "esp32_duktape/module_wifi.h"
#include "esp32_duktape/module_partitions.h"
#include "esp32_duktape/module_os.h"
#include "esp32_mongoose.h"
#include "duktape_utils.h"
#include "duk_trans_socket.h"
#include "sdkconfig.h"

static char tag[] = "modules";

/**
 * The native Console.log() static function.
 */
static duk_ret_t js_console_log(duk_context *ctx) {
	ESP_LOGD(tag, "js_console_log called");
	switch(duk_get_type(ctx, -1)) {
		case DUK_TYPE_STRING: {
			esp32_duktape_console(duk_get_string(ctx, -1));
			break;
		}

		default: {
			duk_to_string(ctx, -1);
			esp32_duktape_console(duk_get_string(ctx, -1));
			break;
		}
	}
  return 0;
} // js_console_log


/**
 * Low level load and evaluate.  The load is performed by making an HTTP
 * get request to obtain the source which is then passed to Duktape eval() for
 * execution.
 */
static duk_ret_t js_esp32_load(duk_context *ctx) {
	ESP_LOGD(tag, "js_esp32_load");
	char *data = curl_client_getContent(duk_get_string(ctx, -1));
	if (data == NULL) {
		ESP_LOGD(tag, "No data");
	} else {
		ESP_LOGD(tag, "we got data");
		int evalResponse = duk_peval_string(ctx, data);
		if (evalResponse != 0) {
			esp32_duktape_console(duk_safe_to_string(ctx, -1));
		}
		duk_pop(ctx);
	}
	free(data);
	return 0;
} //js_esp32_load


typedef struct {
	char *id;
	duk_c_function func;
	int paramCount;
} functionTableEntry_t;


functionTableEntry_t functionTable[] = {
		{"startMongoose",          js_startMongoose, 1},
		{"serverResponseMongoose", js_serverResponseMongoose, 3},
		// Must be last entry
		{NULL, NULL, 0 }
};
/**
 * Retrieve a native function refernce by name.
 * ESP32.getNativeFunction(nativeFunctionID)
 * The input stack contains:
 * [ 0] - String - nativeFunctionID - A string name that is used to lookup a function handle.
 */
static duk_ret_t js_esp32_getNativeFunction(duk_context *ctx) {
	ESP_LOGD(tag, ">> js_esp32_getNativeFunction");
	// Check that the first parameter is a string.
	if (duk_is_string(ctx, 0)) {
		const char *nativeFunctionID = duk_get_string(ctx, 0);
		ESP_LOGD(tag, "- nativeFunctionId that we are looking for is \"%s\"", nativeFunctionID);

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
			ESP_LOGD(tag, "No native found found called %s", nativeFunctionID);
			duk_push_null(ctx);
		}
	} else {
		ESP_LOGD(tag, "No native function id supplied");
		duk_push_null(ctx);
	}
	// We will have either pushed null or a function reference onto the stack.
	ESP_LOGD(tag, "<< js_esp32_getNativeFunction");
	return 1;
} // js_esp32_getNativeFunction


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
	// [0] - New object
	duk_push_object(ctx); // Create new getState object

	// [0] - New object
	// [1] - heap size
	duk_push_number(ctx, (double)esp_get_free_heap_size());

	// [0] - New object
	duk_put_prop_string(ctx, -2, "heapSize"); // Add heapSize to new getState

	return 1;
} // js_esp32_getState


/**
 * Attach the debugger.
 */
static duk_ret_t js_esp32_debug(duk_context *ctx) {
	ESP_LOGD(tag, ">> js_esp32_debug");
	duk_trans_socket_init();
	duk_trans_socket_waitconn();
	ESP_LOGD(tag, "Debugger reconnected, call duk_debugger_attach()");

	duk_debugger_attach(ctx,
		duk_trans_socket_read_cb,
		duk_trans_socket_write_cb,
		duk_trans_socket_peek_cb,
		duk_trans_socket_read_flush_cb,
		duk_trans_socket_write_flush_cb,
		NULL,
		NULL);
	ESP_LOGD(tag, "<< js_esp32_debug");
	return 0;
} // js_esp32_debug


/**
 * Write a log record to the debug output stream.  Exposed
 * as the global log("message").
 */
static duk_ret_t js_global_log(duk_context *ctx) {
	ESP_LOGD("debug", "%s", duk_get_string(ctx, -1));
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


/**
 * Register the ESP32 module with its functions.
 */
static void ModuleESP32(duk_context *ctx) {
	duk_push_global_object(ctx);
	// [0] - Global object

	duk_push_object(ctx); // Create new ESP32 object
	// [0] - Global object
	// [1] - New object

	duk_push_c_function(ctx, js_esp32_load, 1);
	// [0] - Global object
	// [1] - New object
	// [2] - c-function - js_esp32_load

	duk_put_prop_string(ctx, -2, "load"); // Add load to new ESP32
	// [0] - Global object
	// [1] - New object

	duk_push_c_function(ctx, js_esp32_reset, 0);
	// [0] - Global object
	// [1] - New object
	// [2] - c-function - js_esp32_reset

	duk_put_prop_string(ctx, -2, "reset"); // Add reset to new ESP32
	// [0] - Global object
	// [1] - New object

	duk_push_c_function(ctx, js_esp32_getState, 0);
	// [0] - Global object
	// [1] - New object
	// [2] - c-function - js_esp32_getState

	duk_put_prop_string(ctx, -2, "getState"); // Add reset to new ESP32
	// [0] - Global object
	// [1] - New object

	duk_push_c_function(ctx, js_esp32_getNativeFunction, 1);
	// [0] - Global object
	// [1] - New object
	// [2] - c-function - js_esp32_getNativeFunction

	duk_put_prop_string(ctx, -2, "getNativeFunction"); // Add getNativeFunction to new ESP32
	// [0] - Global object
	// [1] - New object

	duk_push_c_function(ctx, js_esp32_debug, 0);
	// [0] - Global object
	// [1] - New object
	// [2] - c-function - js_esp32_debug

	duk_put_prop_string(ctx, -2, "debug"); // Add debug to new ESP32
	// [0] - Global object
	// [1] - New object

	duk_put_prop_string(ctx, -2, "ESP32"); // Add ESP32 to global
	// [0] - Global object

	duk_pop(ctx);
	// <Empty stack>
} // ModuleESP32


/**
 * Register the static modules.  These are modules that will ALWAYS
 * bein the global address space/scope.
 */
void registerModules(duk_context *ctx) {
	duk_idx_t top = duk_get_top(ctx);
	ModuleConsole(ctx);
	assert(top == duk_get_top(ctx));
	ModuleESP32(ctx);
	assert(top == duk_get_top(ctx));
	ModuleFS(ctx);
	assert(top == duk_get_top(ctx));
	ModuleGPIO(ctx);
	assert(top == duk_get_top(ctx));
	ModuleTIMERS(ctx);
	assert(top == duk_get_top(ctx));
	ModuleWIFI(ctx);
	assert(top == duk_get_top(ctx));
	ModuleRMT(ctx);
	assert(top == duk_get_top(ctx));
	ModuleHTTP(ctx);
	assert(top == duk_get_top(ctx));
	ModulePARTITIONS(ctx);
	assert(top == duk_get_top(ctx));
	ModuleMONGOOSE(ctx);
	assert(top == duk_get_top(ctx));
	ModuleOS(ctx);
	assert(top == duk_get_top(ctx));
} // End of registerModules
