#include <duktape.h>
#include <esp_log.h>
#include "esp32_mongoose.h"
#include "telnet.h"
#include "sdkconfig.h"

static char tag[] = "duktape_utils";

// Global flag for whether or not a reset has been request.
static int g_reset = 0;

/**
 * Determine whether or not a reset has been requested.
 */
int esp32_duktape_is_reset() {
	return g_reset;
} // esp32_duktape_is_reset


/**
 * Set the value of the global reset flag.  The value can be determined
 * by a call to esp32_duktape_is_reset
 */
void esp32_duktape_set_reset(int value) {
	g_reset = value;
} // esp32_duktape_is_reset


/**
 * Log a message to the duktape console.
 */
void esp32_duktape_console(const char *message) {
	telnet_esp32_sendData((uint8_t *)message, strlen(message));
	websocket_console_sendData(message);
	ESP_LOGD(tag, "-> %s", message);
} // esp32_duktape_console


/**
 * Add a global function to the context.
 */
void esp32_duktape_addGlobalFunction(
		duk_context *ctx,
		char *functionName,
		duk_c_function func,
		duk_int_t numArgs) {
	duk_push_global_object(ctx);
	duk_push_c_function(ctx, func, numArgs);
	duk_put_prop_string(ctx, -2, functionName);
} // esp32_duktape_addGlobalFunction


/**
 * Dump the value stack to the debug stream.  This is used
 * exclsuively for debugging purposes.
 */
void esp32_duktape_dump_value_stack(duk_context *ctx) {
	duk_idx_t topIdx = duk_get_top(ctx);
	duk_idx_t i;
	ESP_LOGD(tag, "Stack dump of %d items", topIdx);
	for(i=0; i<topIdx; i++) {
		duk_int_t type = duk_get_type(ctx, i);
		switch(type) {
		case DUK_TYPE_BOOLEAN:
			ESP_LOGD(tag, "[%2d] BOOLEAN = %d", i, duk_get_boolean(ctx, i));
			break;
		case DUK_TYPE_BUFFER:
			ESP_LOGD(tag, "[%2d] BUFFER", i);
			break;
		case DUK_TYPE_LIGHTFUNC:
			ESP_LOGD(tag, "[%2d] LIGHTFUNC", i);
			break;
		case DUK_TYPE_NONE:
			ESP_LOGD(tag, "[%2d] NONE", i);
			break;
		case DUK_TYPE_NULL:
			ESP_LOGD(tag, "[%2d] NULL", i);
			break;
		case DUK_TYPE_NUMBER:
			ESP_LOGD(tag, "[%2d] NUMBER = %f", i, duk_get_number(ctx, i));
			break;
		case DUK_TYPE_OBJECT:
			ESP_LOGD(tag, "[%2d] OBJECT", i);
			break;
		case DUK_TYPE_POINTER:
			ESP_LOGD(tag, "[%2d] POINTER", i);
			break;
		case DUK_TYPE_STRING:
			ESP_LOGD(tag, "[%2d] STRING = %s", i, duk_get_string(ctx, i));
			break;
		case DUK_TYPE_UNDEFINED:
			ESP_LOGD(tag, "[%2d] UNDEFINED", i);
			break;
		default:
			ESP_LOGD(tag, "[%2d] *** Unknown ***", i);
			break;
		} // End of switch
	} // End of for loop
} // dumpValueStack
