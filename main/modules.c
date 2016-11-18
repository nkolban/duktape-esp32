#include <esp_log.h>
#include <duktape.h>
#include <stdlib.h>
#include "modules.h"
#include "esp32_duktape/curl_client.h"
#include "duktape_utils.h"
#include "sdkconfig.h"

static char tag[] = "modules";
/**
 * The native Console.log() static function.
 */
static duk_ret_t js_console_log(duk_context *ctx) {
	ESP_LOGD(tag, "js_console_log called");
	switch(duk_get_type(ctx, -1)) {
	case DUK_TYPE_STRING:
		esp32_duktape_console(duk_get_string(ctx, -1));
		break;
	default:
		duk_to_string(ctx, -1);
		esp32_duktape_console(duk_get_string(ctx, -1));
		break;
	}
  return 0;
} // End of js_console_log

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
	}
	free(data);
	return 0;
}

/**
 * Define the static module called "ModuleConsole".
 */
static void ModuleConsole(duk_context *ctx) {
	duk_push_global_object(ctx);
	duk_push_object(ctx); // Create new console object
	duk_push_c_function(ctx, js_console_log, 1);
	duk_put_prop_string(ctx, -2, "log"); // Add log to new console
	duk_put_prop_string(ctx, -2, "console"); // Add console to global
} // ModuleConsole

static void ModuleESP32(duk_context *ctx) {
	duk_push_global_object(ctx);
	duk_push_object(ctx); // Create new ESP32 object
	duk_push_c_function(ctx, js_esp32_load, 1);
	duk_put_prop_string(ctx, -2, "load"); // Add load to new ESP32
	duk_put_prop_string(ctx, -2, "ESP32"); // Add ESP32 to global
} // ModuleESP32

/**
 * Register the static modules.
 */
void registerModules(duk_context *ctx) {
	ModuleConsole(ctx);
	ModuleESP32(ctx);
} // End of registerModules
