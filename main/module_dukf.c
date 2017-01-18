#if defined(ESP_PLATFORM)
#include <esp_log.h>
#include <nvs.h>
#include "sdkconfig.h"
#endif // ESP_PLATFORM

#include <duktape.h>

#include "duktape_utils.h"
#include "logging.h"
#include "dukf_utils.h"
#include "duk_trans_socket.h"

LOG_TAG("module_dukf");

/**
 * Attach the debugger.
 */
static duk_ret_t js_dukf_debug(duk_context *ctx) {
	LOGD(">> js_dukf_debug");
	duk_trans_socket_init();
	duk_trans_socket_waitconn();
	LOGD("Debugger reconnected, call duk_debugger_attach()");

	duk_debugger_attach(ctx,
		duk_trans_socket_read_cb,
		duk_trans_socket_write_cb,
		duk_trans_socket_peek_cb,
		duk_trans_socket_read_flush_cb,
		duk_trans_socket_write_flush_cb,
		NULL,
		NULL,
		NULL);
	LOGD("<< js_dukf_debug");
	return 0;
} // js_esp32_debug


// Ask JS to perform a gabrage collection.
static duk_ret_t js_dukf_gc(duk_context *ctx) {
	duk_gc(ctx, 0);
	return 0;
} // js_dukf_gc


// Return the global object.
static duk_ret_t js_dukf_global(duk_context *ctx) {
	duk_push_global_object(ctx);
	return 1;
} // js_dukf_global


/**
 * Load a file.  The result is pushed onto the stack as a string.
 */
static duk_ret_t js_dukf_loadFile(duk_context *ctx) {
	//dukf_log_heap("js_dukf_loadFile");
	size_t fileSize;
	char *fileName = (char *)duk_get_string(ctx, -1);
	const char *data = dukf_loadFileFromESPFS(fileName, &fileSize);
	if (data == NULL) {
		duk_push_null(ctx);
	} else {
		duk_push_lstring(ctx, data, fileSize);
	}
	//dukf_log_heap("js_dukf_loadFile");
	return 1;
} // js_dukf_loadFile


/*
 * Log the heap size with a tag.
 * [0] - tag
 */
static duk_ret_t js_dukf_logHeap(duk_context *ctx) {
	const char *tag = duk_get_string(ctx, -1);
	dukf_log_heap(tag);
	return 0;
} // js_dukf_logHeap


/**
 * Run the contents of the names file.
 * [0] - fileName
 */
static duk_ret_t js_dukf_runFile(duk_context *ctx) {
	const char *fileName = duk_get_string(ctx, -1);
	dukf_runFile(ctx, fileName);
	return 0;
} // js_dukf_runFile


/*
 * Set the named file as the start file.  In our logic, after initialization, we
 * check the value of the NVS esp32duktape->start for existence and for a string
 * file name.  If this key has a value then we try and run that script.
 *
 * [0] - string - fileName to try and run at startup.
 */
static duk_ret_t js_dukf_setStartFile(duk_context *ctx) {
	const char *fileName = duk_get_string(ctx, -1);
	nvs_handle handle;
	nvs_open("esp32duktape", NVS_READWRITE, &handle);
	nvs_set_str(handle, "start", fileName);
	nvs_commit(handle);
	nvs_close(handle);
	return 0;
} // js_dukf_setStartFile


void ModuleDUKF(duk_context *ctx) {
	duk_push_global_object(ctx);
	// [0] - Global object

	duk_push_object(ctx); // Create new DUKF object
	// [0] - Global object
	// [1] - New object - DUKF Object

	ADD_FUNCTION("debug",        js_dukf_debug,         1);
	ADD_FUNCTION("gc",           js_dukf_gc,            1);
	ADD_FUNCTION("global",       js_dukf_global,        0);
	ADD_FUNCTION("loadFile",     js_dukf_loadFile,      1);
	ADD_FUNCTION("logHeap",      js_dukf_logHeap,       1);
	ADD_FUNCTION("runFile",      js_dukf_runFile,       1);
	ADD_FUNCTION("setStartFile", js_dukf_setStartFile,  1);


#if defined(ESP_PLATFORM)
	duk_push_string(ctx, "/spiffs");
#else // ESP_PLATFORM
	duk_push_string(ctx, "/home/kolban/esp32/esptest/apps/workspace/duktape/filesystem");
#endif // ESP_PLATFORM
	duk_put_prop_string(ctx, -2, "FILE_SYSTEM_ROOT"); // Add FILE_SYSTEM_ROOT to DUKF


// Define the global DUKF.OS to be either "ESP32" or "Linux"
#if defined(ESP_PLATFORM)
	duk_push_string(ctx, "ESP32");
#else // ESP_PLATFORM
	duk_push_string(ctx, "Linux");
#endif // ESP_PLATFORM
	duk_put_prop_string(ctx, -2, "OS");


	duk_put_prop_string(ctx, -2, "DUKF"); // Add DUKF to global
	// [0] - Global object

	duk_pop(ctx);
	// <Empty stack>
} // ModuleDUKF
