#include "esp32_duktape/module_http.h"
#include <esp_log.h>
#include "sdkconfig.h"

static char tag[] = "module_http";

/**
 * The options parameter is an object which contains the details of
 * the request to be made:
 * This includes:
 * {
 *    protocol: <String> - default http
 *    hostname: <String> - name of host
 *    port: <number> - port number - defaults to 80
 *    method: <String> - http method.  Defaults to "GET"
 *    path: <String> - path string.  Defaults to "/"
 * }
 * [0] - HTTP request options
 * [1] - Callback function
 */
static duk_ret_t js_http_request(duk_context *ctx) {
	ESP_LOGD(tag, ">> js_http_request");
	ESP_LOGD(tag, "<< js_http_request");
	return 0;
}

/**
 * Create the HTTP module in Global.
 */
void ModuleHTTP(duk_context *ctx) {
	duk_push_global_object(ctx);
	// [0] - Global object

	duk_idx_t idx = duk_push_object(ctx); // Create new HTTP object
	// [0] - Global object
	// [1] - New object - HTTP object

	duk_push_c_function(ctx, js_http_request, 2);
	// [0] - Global object
	// [1] - New object - HTTP object
	// [2] - C Function - js_rmt_txConfig

	duk_put_prop_string(ctx, idx, "requestC"); // Add something to new HTTP
	// [0] - Global object
	// [1] - New object - HTTP object

	duk_put_prop_string(ctx, 0, "HTTP"); // Add HTTP to global
	// [0] - Global object

	duk_pop(ctx);
	// <Empty Stack>
} // ModuleHTTP
