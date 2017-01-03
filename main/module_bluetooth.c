#include <bt.h>
#include <duktape.h>
#include <esp_bt_main.h>
#include <esp_gap_ble_api.h>
#include <esp_log.h>
#include <stdio.h>
#include <string.h>

#include "logging.h"
#include "module_bluetooth.h"

LOG_TAG("module_bluetooth");
static void gap_event_handler(uint32_t event, void *param) {
	LOGD(">> gap_event_handler");
	LOGD("<< gap_event_handler");
} // gap_event_handler


static duk_ret_t js_bluetooth_init(duk_context *ctx) {
	LOGD(">> init_bluetooth");
	int errRc;
	bt_controller_init();
	esp_init_bluetooth();
	esp_enable_bluetooth();
	errRc = esp_ble_gap_register_callback(gap_event_handler);
	if (errRc != ESP_OK) {
		LOGE("esp_ble_gap_register_callback: rc=%d", errRc);
	}
	LOGD("<< init_bluetooth");
	return 0;
} // init_bluetooth


/**
 * Add native methods to the Bluetooth object.
 * [0] - Bluetooth Object
 */
duk_ret_t ModuleBluetooth(duk_context *ctx) {
	int idx = -2;
	duk_push_c_function(ctx, js_bluetooth_init, 0);
	// [0] - Bluetooth object
	// [1] - C Function - js_bluetooth_init

	duk_put_prop_string(ctx, idx, "init"); // Add init to Bluetooth
	// [0] - Bluetooth object

	duk_pop(ctx);
	// <Empty Stack>
	return 0;
} // ModuleBluetooth
