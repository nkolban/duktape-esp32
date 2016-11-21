/**
 * Implement the timers functions.
 *
 * This file provides the implementation for the following global functions:
 * * setInterval
 * * setTimeout
 * * clearInterval
 * * clearTimeout
 *
 */
#include <stdbool.h>
#include <esp_log.h>
#include <esp_system.h>
#include <duktape.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include "duktape_utils.h"
#include "sdkconfig.h"

static char tag[] = "module_timers";

typedef struct {
	void *func; // Function to invoke when the timer fires.
	unsigned long duration; // The duation to wait for (relative).
	unsigned long wakeTime; // The real time to wake up at (absolute).
	uint8_t interval; // True if this is an interval timer, false if it is a timeout timer;.
	unsigned long id; // Id of the timer.
} timer_record_t;

static duk_ret_t js_timers_clearInterval(duk_context *ctx) {
	ESP_LOGD(tag, ">> js_timers_clearInterval");
	ESP_LOGD(tag, "Not implemented");
	ESP_LOGD(tag, "<< js_timers_clearInterval");
	return 0;
} // js_timers_clearInterval


static duk_ret_t js_timers_clearTimeout(duk_context *ctx) {
	ESP_LOGD(tag, ">> js_timers_clearTimeout");
	ESP_LOGD(tag, "Not implemented");
	ESP_LOGD(tag, "<< js_timers_clearTimeout");
	return 0;
} // js_timers_clearTimeout


static duk_ret_t js_timers_setInterval(duk_context *ctx) {
	ESP_LOGD(tag, ">> js_timers_setInterval");
	ESP_LOGD(tag, "Not implemented");
	ESP_LOGD(tag, "<< js_timers_setInterval");
	return 0;
} // js_timers_setInterval


static duk_ret_t js_timers_setTimeout(duk_context *ctx) {
	ESP_LOGD(tag, ">> js_timers_setTimeout");
	ESP_LOGD(tag, "Not implemented");
	ESP_LOGD(tag, "<< js_timers_setTimeout");
	return 0;
} // js_timers_setTimeout


/**
 * Create the Timers functions in Global.
 */
void ModuleTIMERS(duk_context *ctx) {
	duk_push_global_object(ctx);

	duk_push_c_function(ctx, js_timers_clearInterval, 1);
	duk_put_prop_string(ctx, -2, "clearInterval"); // Add clearInterval to global

	duk_push_c_function(ctx, js_timers_clearTimeout, 1);
	duk_put_prop_string(ctx, -2, "clearTimeout"); // Add clearTimeout to global

	duk_push_c_function(ctx, js_timers_setInterval, 2);
	duk_put_prop_string(ctx, -2, "setInterval"); // Add setInterval to global

	duk_push_c_function(ctx, js_timers_setTimeout, 2);
	duk_put_prop_string(ctx, -2, "setTimeout"); // Add setTimeout to global
} // ModuleTIMERS
