#include <duktape.h>
#include <esp_log.h>
#include "telnet.h"
#include "sdkconfig.h"

static char tag[] = "duktape_utils";

/**
 * Log a message to the duktape console.
 */
void esp32_duktape_console(const char *message) {
	telnet_esp32_sendData((uint8_t *)message, strlen(message));
	ESP_LOGD(tag, "-> %s", message);
} // End of esp32_duktape_console

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
} // End of esp32_duktape_addGlobalFunction
