#ifdef ESP_PLATFORM
#include <esp_log.h>
#include "sdkconfig.h"
#endif
#include <duktape.h>
#include "logging.h"
#include "dukf_utils.h"
#include "duk_trans_socket.h"

static char tag[] = "module_dukfc";

static duk_ret_t js_dukf_loadFile(duk_context *ctx) {
	size_t fileSize;
	char *fileName = (char *)duk_get_string(ctx, -1);
	char *data = dukf_loadFile(fileName, &fileSize);
	if (data == NULL) {
		duk_push_null(ctx);
	} else {
		duk_push_lstring(ctx, data, fileSize);
	}
	return 1;
} // js_dukf_loadFile


// Ask JS to perform a gabrage collection.
static duk_ret_t js_dukf_gc(duk_context *ctx) {
	duk_gc(ctx, 0);
	return 0;
} // js_dukf_gc

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

void ModuleDUKF(duk_context *ctx) {
	duk_push_global_object(ctx);
	// [0] - Global object

	duk_push_object(ctx); // Create new DUKF object
	// [0] - Global object
	// [1] - New object - DUKF Object

	duk_push_c_function(ctx, js_dukf_loadFile, 1);
	// [0] - Global object
	// [1] - New object - DUKF Object
	// [2] - C Function - js_dukf_loadFile

	duk_put_prop_string(ctx, -2, "loadFile"); // Add loadFile to new DUKF
	// [0] - Global object
	// [1] - New object - DUKF Object

	duk_push_c_function(ctx, js_dukf_debug, 1);
	// [0] - Global object
	// [1] - New object - DUKF Object
	// [2] - C Function - js_dukf_debug

	duk_put_prop_string(ctx, -2, "debug"); // Add debug to new DUKF
	// [0] - Global object
	// [1] - New object - DUKF Object

	duk_push_c_function(ctx, js_dukf_gc, 1);
	// [0] - Global object
	// [1] - New object - DUKF Object
	// [2] - C Function - js_dukf_gc

	duk_put_prop_string(ctx, -2, "gc"); // Add gc to new DUKF
	// [0] - Global object
	// [1] - New object - DUKF Object

#ifdef ESP_PLATFORM
	duk_push_string(ctx, "/spiffs");
#else
	duk_push_string(ctx, "/home/kolban/esp32/esptest/apps/workspace/duktape/spiffs");
#endif
	duk_put_prop_string(ctx, -2, "FILE_SYSTEM_ROOT"); // Add FILE_SYSTEM_ROOT to DUKF

	duk_put_prop_string(ctx, -2, "DUKF"); // Add DUKF to global
	// [0] - Global object

	duk_pop(ctx);
	// <Empty stack>
} // ModuleDUKF
