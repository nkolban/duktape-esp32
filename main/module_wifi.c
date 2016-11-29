/**
 * Handle WiFi functions for ESP32-Duktape.  Functions included are:
 *
 * * WIFI.scan(callback) - Scan for WiFi access points and when complete, invoke the callback with
 * an array of found access points.  Each record in the list of access points will include:
 *   * ssid
 *   * rssi
 *   * mac
 */
#include "esp32_duktape/module_wifi.h"
#include <duktape.h>

static duk_ret_t js_wifi_scan(duk_context *ctx) {
	return 0;
}

/**
 * Create the WIFI module in Global.
 */
void ModuleWIFI(duk_context *ctx) {

	// [0] - Global object
	duk_push_global_object(ctx);

	// [0] - Global object
	// [1] - New object
	duk_idx_t idx = duk_push_object(ctx); // Create new WIFI object

	// [0] - Global object
	// [1] - New object
	// [2] - c-func - js_wifi_scan
	duk_push_c_function(ctx, js_wifi_scan, 0);

	// [0] - Global object
	// [1] - New object
	duk_put_prop_string(ctx, idx, "scan"); // Add scan to new WIFI

	// [0] - Global object
	duk_put_prop_string(ctx, 0, "WIFI"); // Add WIFI to global

	duk_pop(ctx);
} // ModuleWIFI
