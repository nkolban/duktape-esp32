#include <duktape.h>
#include <esp_log.h>
#include <sys/time.h>
#include "esp32_mongoose.h"
#include "telnet.h"
#include "sdkconfig.h"

static char tag[] = "duktape_utils";

#define CALLBACK_STASH_OBJECT_NAME "callbackStash"

// Global flag for whether or not a reset has been request.
static int g_reset = 0;

static uint32_t g_stashCounter = 1;

void esp32_duktape_stash_init(duk_context *ctx) {
	g_stashCounter = 1;
	duk_push_global_object(ctx);
	// [0] - Global object

	duk_push_object(ctx);
	// [0] - Global object
	// [1] - New object

	duk_put_prop_string(ctx, -2, CALLBACK_STASH_OBJECT_NAME);
	// [0] - Global object

	duk_pop(ctx);
	// <Empty Stack>
} // esp32_duktape_stash_init

void esp32_duktape_unstash_object(duk_context *ctx, uint32_t key) {
	duk_push_global_object(ctx);
	// [0] - Global object

	if (duk_get_prop_string(ctx, -1, CALLBACK_STASH_OBJECT_NAME) ==0) {
		// [0] - Global object
		// [1] - Undefined
		ESP_LOGE(tag, "esp32_duktape_unstash_object: No such object called \"%s\"", CALLBACK_STASH_OBJECT_NAME);
		duk_remove(ctx, -2);
		// [0] - Undefined
		return;
	}
	// [0] - Global object
	// [1] - CALLBACK_STASH_OBJECT_NAME object

	duk_push_int(ctx, key);
	// [0] - Global object
	// [1] - CALLBACK_STASH_OBJECT_NAME object
	// [2] - key

	if (duk_get_prop(ctx, -1) == 0) {
		// [0] - Global object
		// [1] - CALLBACK_STASH_OBJECT_NAME object
		// [2] - Undefined

		ESP_LOGE(tag, "esp32_duktape_unstash_object: No such stash key: %d", key);

		duk_remove(ctx, -2);
		// [0] - Global object
		// [1] - Undefined

		duk_remove(ctx, -2);
		// [0] - Undefined
		return;
	}
	// [0] - Global object
	// [1] - CALLBACK_STASH_OBJECT_NAME object
	// [2] - Previously stashed object

	duk_remove(ctx, -2);
	// [0] - Global object
	// [1] - Previously stashed object

	duk_remove(ctx, -2);
	// [0] - Previously stashed object

} // esp32_duktape_unstash_object


void esp32_duktape_unstash_array(duk_context *ctx, uint32_t key) {
	duk_size_t length;
	int i;
	duk_idx_t objIdx;
	duk_idx_t delIdx;

	// Push the global onto the stack.
	// Push the CALLBACK_STASH_OBJECT_NAME object onto the stack.
	// Push each of the array elements onto the stack.
	// Delete the global and CALLBACK_STASH_OBJECT_NAME object from the stack.
	duk_push_global_object(ctx);
	// [0] - Global object

	delIdx = duk_get_top_index(ctx);
	// [0] - Global object

	if (duk_get_prop_string(ctx, -1, CALLBACK_STASH_OBJECT_NAME) == 0) {
		// [0] - Global object
		// [1] - Undefined
		ESP_LOGE(tag, "esp32_duktape_unstash_array: No such object called \"%s\"", CALLBACK_STASH_OBJECT_NAME);
		duk_pop_2(ctx);
		// <Empty Stack>
		return;
	}
	// [0] - Global object
	// [1] - CALLBACK_STASH_OBJECT_NAME object

	duk_push_int(ctx, key);
	// [0] - Global object
	// [1] - CALLBACK_STASH_OBJECT_NAME object
	// [2] - key

	duk_get_prop(ctx, -1);
	// [0] - Global object
	// [1] - CALLBACK_STASH_OBJECT_NAME object
	// [2] - Callback array

	objIdx = duk_get_top_index(ctx);

	length = duk_get_length(ctx, -1);
	for (i=length-1; i>=0; i--) {
		duk_get_prop_index(ctx, objIdx, i);
	}
	// [0] - Global object
	// [1] - CALLBACK_STASH_OBJECT_NAME object
	// [2] - Callback array
	// [3] - item ...
	// [.] - item .
	// [end] - item

	duk_remove(ctx, delIdx);
	duk_remove(ctx, delIdx);
	duk_remove(ctx, delIdx);

	// [0] - item ...
	// [.] - item .
	// [end] - item
}


/**
 * Delete a stashed object that was previously stashed and returned
 * the access key passed as a parameter.
 */
void esp32_duktape_stash_delete(duk_context *ctx, uint32_t key) {
	duk_push_global_object(ctx);
	// [0] - Global

	if (duk_get_prop_string(ctx, -1, CALLBACK_STASH_OBJECT_NAME) == 0) {
		// [0] - Global
		// [1] - undefined
		ESP_LOGE(tag, "esp32_duktape_stash_delete: No such object called \"%s\"", CALLBACK_STASH_OBJECT_NAME);
		duk_pop_2(ctx);
		// Empty stack
		return;
	}
	// [0] - Global
	// [1] - CALLBACK_STASH_OBJECT_NAME object

	duk_push_int(ctx, key);
	// [0] - Global
	// [1] - CALLBACK_STASH_OBJECT_NAME object
	// [2] - key

	duk_del_prop(ctx, -2);
	// [0] - Global
	// [1] - CALLBACK_STASH_OBJECT_NAME object

	duk_pop_2(ctx);
	// Empty stack
} // esp32_duktape_stash_delete


/**
 * Stash an object that is on the top of the stack and return an access key.
 * [0] - Object to stash
 *
 * On return, the stack is empty and the return value is the key to the new stash.
 */
uint32_t esp32_duktape_stash_object(duk_context *ctx, int count) {
	if (duk_is_object(ctx, -1) == 0) {
		ESP_LOGE(tag, "esp32_duktape_stash_object: top of stack is not an object.");
		return 0;
	}

	duk_push_global_object(ctx);
	// [0] - object to stash
	// [1] - global object

	if (duk_get_prop_string(ctx, -1, CALLBACK_STASH_OBJECT_NAME) ==0) {
		// [0] - object to stash
		// [1] - global
		// [2] - undefined
		ESP_LOGE(tag, "esp32_duktape_stash_object: No such object called \"%s\"", CALLBACK_STASH_OBJECT_NAME);
		duk_pop_3(ctx);
		// Empty stack
		return 0;
	}
	// [0] - object to stash
	// [1] - global
	// [2] - callbackStash

	uint32_t stashKey = g_stashCounter;
	g_stashCounter++;

	duk_push_int(ctx, stashKey);
	// [0] - object to stash
	// [1] - global
	// [2] - callbackStash
	// [3] - key

	duk_dup(ctx, -4);
	// [0] - object to stash
	// [1] - global
	// [2] - callbackStash
	// [3] - key
	// [4] - object to stash

	duk_put_prop(ctx, -3);
	// [0] - object to stash
	// [1] - global
	// [2] - callbackStash

	duk_pop_3(ctx);
	// <Empty Stack>

	return stashKey;
} // esp32_duktape_stash_object


/**
 * The top <count> items on the stack are added to an array.
 */
uint32_t esp32_duktape_stash_array(duk_context *ctx, int count) {
	// [0] - Item 1
	// [.] - ...
	// [count-1] - Item Count

	int i;
	if (duk_get_top(ctx) < count) {
		ESP_LOGE(tag, "Can't stash %d items when only %d items on value stack.", count, duk_get_top(ctx));
		return 0;
	}
	duk_idx_t arrayIdx = duk_get_top(ctx) - count;

	duk_push_array(ctx);
	// [0] - Item 1
	// [.] - ...
	// [count-1] - Item Count
	// [count] - array

	duk_insert(ctx, arrayIdx);
	// [0] - array
	// [1] - Item 1
	// [.] - ...
	// [count] - Item Count
	for (i=0; i<count; i++) {
		duk_put_prop_index(ctx, arrayIdx, i);
	}
	// [0] - array

	duk_push_global_object(ctx);
	// [0] - array
	// [1] - global

	if (duk_get_prop_string(ctx, -1, CALLBACK_STASH_OBJECT_NAME) == 0) {
		// [0] - array
		// [1] - global
		// [2] - undefined
		ESP_LOGE(tag, "esp32_duktape_stash_array: No such object called \"%s\"", CALLBACK_STASH_OBJECT_NAME);
		duk_pop_3(ctx);
		// Empty stack
		return 0;
	}
	// [0] - array
	// [1] - global
	// [2] - callbackStash

	uint32_t stashKey = g_stashCounter;
	g_stashCounter++;

	duk_push_int(ctx, stashKey);
	// [0] - array
	// [1] - global
	// [2] - callbackStash
	// [3] - key

	duk_dup(ctx, -4);
	// [0] - array
	// [1] - global
	// [2] - callbackStash
	// [3] - key
	// [4] - array

	duk_put_prop(ctx, -3);
	// [0] - array
	// [1] - global
	// [2] - callbackStash

	duk_pop_3(ctx);
	// Empty stack

	return stashKey;
} // esp32_duktape_stash


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
	ESP_LOGD(tag, "Console: %s", message);
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
 * exclusively for debugging purposes.
 */
void esp32_duktape_dump_value_stack(duk_context *ctx) {
	duk_idx_t topIdx = duk_get_top(ctx);
	duk_idx_t i;
	ESP_LOGD(tag, ">> Stack dump of %d items", topIdx);
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

		case DUK_TYPE_OBJECT:{
			if (duk_is_function(ctx, i)) {
				ESP_LOGD(tag, "[%2d] OBJECT - Function", i);
			} else {
				duk_dup(ctx, i);
				const char *json = duk_json_encode(ctx, -1);
				ESP_LOGD(tag, "[%2d] OBJECT - %s", i, json!=NULL?json:"NULL");
				duk_pop(ctx);
			}
			break;
		}

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
	ESP_LOGD(tag, "<< End stack dump.")
} // dumpValueStack


/**
 * This function must return the number of milliseconds since the 1970
 * epoch.  We use getttimeofday() provided by the environment.  The name
 * of the function must be specified in duk_config.h ... for example:
 *
 * #define DUK_USE_DATE_GET_NOW(ctx) esp32_duktape_get_now()
 * #define DUK_USE_DATE_GET_LOCAL_TZOFFSET(d)  0
 */
duk_double_t esp32_duktape_get_now() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	duk_double_t ret = floor(tv.tv_sec * 1000 + tv.tv_usec/1000);
	return ret;
} // esp32_duktape_get_now
