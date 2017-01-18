#include "sdkconfig.h"
#if defined(CONFIG_BT_ENABLED)

#include <bt.h>
#include <duktape.h>
#include <esp_bt_main.h>
#include <esp_gap_ble_api.h>
#include <esp_log.h>
#include <stdio.h>
#include <string.h>

#include "duktape_utils.h"
#include "logging.h"
#include "module_bluetooth.h"

LOG_TAG("module_bluetooth");
static void gap_event_handler(
	esp_gap_ble_cb_event_t event,
	esp_ble_gap_cb_param_t *param
) {
	LOGD(">> gap_event_handler");
	LOGD("<< gap_event_handler");
} // gap_event_handler


static duk_ret_t js_bluetooth_init(duk_context *ctx) {
	LOGD(">> init_bluetooth");
	int errRc;
	esp_bt_controller_init();
	esp_bluedroid_init();
	esp_bluedroid_enable();
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
	ADD_FUNCTION("init", js_bluetooth_init, 0);
	return 0;
} // ModuleBluetooth
#endif
