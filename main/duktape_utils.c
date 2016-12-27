#ifdef ESP_PLATFORM
#include <esp_log.h>
#include "telnet.h"
#include "sdkconfig.h"
#endif

#include <duktape.h>
#include <sys/time.h>
#include "duktape_utils.h"
#include "logging.h"

LOG_TAG("duktape_utils");

#define CALLBACK_STASH_OBJECT_NAME "_dukf_callbackStash"

// Global flag for whether or not a reset has been request.
static int g_reset = 0;

// Global counter for the next "stash" key.
static uint32_t g_stashCounter = 1;

static uint32_t getNextStashKey();


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
 * Log a message to the duktape console.
 */
void esp32_duktape_console(const char *message) {
	LOGD_TAG("log", "Console: %s", message);
} // esp32_duktape_console


/*
 * On the call stack will be either a string or a buffer. This function
 * retrieves the data associated with it and returns it along with the
 * length.
 *
 * * idx - The Index of the buffer or string
 * * size - A pointer to the size to be filled in.
 *
 * Returns NULL if no data is available.
 */
const void *esp32_duktape_dataFromStringOrBuffer(
	duk_context *ctx, duk_idx_t idx, size_t *size) {
	if (duk_is_string(ctx, idx)) {
		const char *data = duk_get_string(ctx, idx);
		*size = strlen(data);
		return data;
	}
	return duk_get_buffer(ctx, idx, size);
} // esp32_duktape_dataFromStringOrBuffer


/**
 * Dump the value stack to the debug stream.  This is used
 * exclusively for debugging purposes.
 */
void esp32_duktape_dump_value_stack(duk_context *ctx) {
	duk_idx_t topIdx = duk_get_top(ctx);
	LOGD(">> Stack dump of %d items", topIdx);
	duk_idx_t i;
	for(i=0; i<topIdx; i++) {
		duk_int_t type = duk_get_type(ctx, i);
		switch(type) {
		case DUK_TYPE_BOOLEAN:
			LOGD("[%2d] BOOLEAN = %d", i, duk_get_boolean(ctx, i));
			break;
		case DUK_TYPE_BUFFER:
			LOGD("[%2d] BUFFER", i);
			break;
		case DUK_TYPE_LIGHTFUNC:
			LOGD("[%2d] LIGHTFUNC", i);
			break;
		case DUK_TYPE_NONE:
			LOGD("[%2d] NONE", i);
			break;
		case DUK_TYPE_NULL:
			LOGD("[%2d] NULL", i);
			break;
		case DUK_TYPE_NUMBER:
			LOGD("[%2d] NUMBER = %f", i, duk_get_number(ctx, i));
			break;

		case DUK_TYPE_OBJECT:{
			if (duk_is_function(ctx, i)) {
				LOGD("[%2d] OBJECT - Function", i);
			} else {
				duk_dup(ctx, i);
				const char *json = duk_json_encode(ctx, -1);
				LOGD("[%2d] OBJECT - %s", i, json!=NULL?json:"NULL");
				duk_pop(ctx);
			}
			break;
		}

		case DUK_TYPE_POINTER:
			LOGD("[%2d] POINTER", i);
			break;
		case DUK_TYPE_STRING:
			LOGD("[%2d] STRING = %s", i, duk_get_string(ctx, i));
			break;
		case DUK_TYPE_UNDEFINED:
			LOGD("[%2d] UNDEFINED", i);
			break;
		default:
			LOGD("[%2d] *** Unknown ***", i);
			break;
		} // End of switch
	} // End of for loop
	LOGD("<< End stack dump.");
} // dumpValueStack


/**
 * This function must return the number of milliseconds since the 1970
 * epoch.  We use gettimeofday() provided by the environment.  The name
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


/**
 * Determine whether or not a reset has been requested.
 */
int esp32_duktape_is_reset() {
	return g_reset;
} // esp32_duktape_is_reset

/**
 *  Log a Duktape Error.
 *  Property
 *  name	  Compatibility	Description
 *  name	     standard	 Name of error, e.g. TypeError, inherited
 *  message	   standard	 Optional message of error, own property, empty message inherited if absent
 *  fileName	 Rhino     Filename related to error source, inherited accessor
 *  lineNumber Rhino     Linenumber related to error source, inherited accessor
 *  stack	     V8        Traceback as a multi-line human redable string, inherited accessor
 */
void esp32_duktape_log_error(duk_context *ctx) {
	duk_idx_t errObjIdx = duk_get_top_index(ctx);
	duk_get_prop_string(ctx, errObjIdx, "name");
	duk_get_prop_string(ctx, errObjIdx, "message");
	duk_get_prop_string(ctx, errObjIdx, "lineNumber");
	duk_get_prop_string(ctx, errObjIdx, "stack");
	// [0] - Err object
	// [1] - name
	// [2] - message
	// [3] - lineNumber
	// [4] - Stack

	const char *name = duk_get_string(ctx, -4);
	const char *message = duk_get_string(ctx, -3);
	int lineNumber = duk_get_int(ctx, -2);
	const char *stack = duk_get_string(ctx, -1);
	LOGD("Error: %s: %s\nline: %d\nStack: %s",
			name!=NULL?name:"NULL",
			message!=NULL?message:"NULL",
			lineNumber,
			stack!=NULL?stack:"NULL");
	duk_pop_n(ctx, 4);
	// [0] - Err
} // esp32_duktape_log_error

/**
 * Set the value of the global reset flag.  The value can be determined
 * by a call to esp32_duktape_is_reset
 */
void esp32_duktape_set_reset(int value) {
	g_reset = value;
} // esp32_duktape_is_reset


/**
 * The top <count> items on the stack are added to an array and the array is stashed.
 * The value stack is empty at completion.
 */
uint32_t esp32_duktape_stash_array(duk_context *ctx, int count) {
	// [0] - Item 1
	// [.] - ...
	// [count-1] - Item Count


	if (duk_get_top(ctx) < count) {
		LOGE("Can't stash %d items when only %d items on value stack.", count, duk_get_top(ctx));
		return 0;
	}

	duk_idx_t arrayIdx = duk_get_top_index(ctx);

	duk_push_array(ctx);
	// [0] - Item 1
	// [.] - ...
	// [count-1] - Item Count
	// [count] - array

	// Move the top of the stack to arrayIdx.
	duk_insert(ctx, arrayIdx);
	// [0] - array
	// [1] - Item 1
	// [.] - ...
	// [count] - Item Count

	int i;
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
		LOGE("esp32_duktape_stash_array: No such object called \"%s\"", CALLBACK_STASH_OBJECT_NAME);
		duk_pop_3(ctx);
		// Empty stack
		return 0;
	}
	// [0] - array
	// [1] - global
	// [2] - callbackStash

	uint32_t stashKey = getNextStashKey(); 	// Get the new stash key

	duk_push_int(ctx, stashKey);
	// [0] - array
	// [1] - global
	// [2] - callbackStash
	// [3] - stashKey

	duk_dup(ctx, -4);
	// [0] - array
	// [1] - global
	// [2] - callbackStash
	// [3] - stashKey
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
 * Delete a stashed object that was previously stashed and returned
 * the access key passed as a parameter.
 */
void esp32_duktape_stash_delete(duk_context *ctx, uint32_t key) {
	duk_push_global_object(ctx);
	// [0] - Global

	if (duk_get_prop_string(ctx, -1, CALLBACK_STASH_OBJECT_NAME) == 0) {
		// [0] - Global
		// [1] - undefined
		LOGE("esp32_duktape_stash_delete: No such object called \"%s\"", CALLBACK_STASH_OBJECT_NAME);
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

	// Delete the property in CALLBACK_STASH_OBJECT_NAME object called "key"
	duk_del_prop(ctx, -2);
	// [0] - Global
	// [1] - CALLBACK_STASH_OBJECT_NAME object

	duk_pop_2(ctx);
	// Empty stack
} // esp32_duktape_stash_delete

/**
 * Initialize the stash environment.  This must be called before any other
 * stash functions.  We create a global variable that is defined in
 * CALLBACK_STASH_OBJECT_NAME that is an object.
 */
void esp32_duktape_stash_init(duk_context *ctx) {
	g_stashCounter = 1; // Initialize the handle/counter for the next stash key to be returned.

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


/**
 *  Unstash a previously stashed array object.  Return the number of new
 *  elements on the value stack.
 *  * key - the key to a previously stashed array object.
 */
size_t esp32_duktape_unstash_array(duk_context *ctx, uint32_t key) {
	// Push the global onto the stack.
	// Push the CALLBACK_STASH_OBJECT_NAME object onto the stack.
	// Push each of the array elements onto the stack.
	// Delete the global and CALLBACK_STASH_OBJECT_NAME object from the stack.
	duk_push_global_object(ctx);
	// [0] - Global object

	duk_idx_t delIdx = duk_get_top_index(ctx);
	// [0] - Global object

	if (duk_get_prop_string(ctx, -1, CALLBACK_STASH_OBJECT_NAME) == 0) {
		// [0] - Global object
		// [1] - Undefined
		LOGE("esp32_duktape_unstash_array: No such object called \"%s\"", CALLBACK_STASH_OBJECT_NAME);
		duk_pop_2(ctx);
		// <Empty Stack>
		return 0;
	}
	// [0] - Global object
	// [1] - CALLBACK_STASH_OBJECT_NAME object

	if (duk_get_prop_index(ctx, -1, key) == 0) {
		// [0] - Global object
		// [1] - CALLBACK_STASH_OBJECT_NAME object
		// [2] - [undefined]
		// We were unable to find a stashed value with the given stash key.
		LOGE("Unable to find a stashed array with key: %d", key);
		duk_pop_3(ctx);
		return 0;
	}
	// [0] - Global object
	// [1] - CALLBACK_STASH_OBJECT_NAME object
	// [2] - Callback array

	duk_idx_t objIdx = duk_get_top_index(ctx);

	duk_size_t arraySize = duk_get_length(ctx, -1);

	int i;
	for (i=arraySize-1; i>=0; i--) {
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
	return arraySize;
} // esp32_duktape_unstash_array


/**
 * Stash an object/item that is on the top of the stack and return an access key.
 * [0] - Object/item to stash
 *
 * On return, the stack is empty and the return value is the key to the new stash entry.
 */
uint32_t esp32_duktape_stash_object(duk_context *ctx) {

	duk_push_global_object(ctx);
	// [0] - object to stash
	// [1] - global object

	if (duk_get_prop_string(ctx, -1, CALLBACK_STASH_OBJECT_NAME) ==0) {
		// [0] - object to stash
		// [1] - global
		// [2] - undefined
		LOGE("esp32_duktape_stash_object: No such object called \"%s\"", CALLBACK_STASH_OBJECT_NAME);
		duk_pop_3(ctx);
		// Empty stack
		return 0;
	}
	// [0] - object to stash
	// [1] - global
	// [2] - callbackStash

	uint32_t stashKey = getNextStashKey(); 	// Get the new stash key

	duk_push_int(ctx, stashKey);
	// [0] - object to stash
	// [1] - global
	// [2] - callbackStash
	// [3] - stashKey

	duk_dup(ctx, -4);
	// [0] - object to stash
	// [1] - global
	// [2] - callbackStash
	// [3] - stashKey
	// [4] - object to stash

	// Put prop takes the top two items on the stack.  The 1st is the key
	// the 2nd is the value.  It then creates/updates the property defined by the key
	// to the value on the object supplied in the parameter index.
	//
	// In this call, we end up with <callbackStashObject>[key] = object to stash.
	//
	duk_put_prop(ctx, -3);
	// [0] - object to stash
	// [1] - global
	// [2] - callbackStash

	duk_pop_3(ctx);
	// <Empty Stack>

	return stashKey;
} // esp32_duktape_stash_object

/**
 * Get the next stash key.
 * Return a unique integer value that is the stash key to return for
 * someone who needs a new stash key value.
 */
static uint32_t getNextStashKey() {
	uint32_t stashKey = g_stashCounter;
	g_stashCounter++;
	return stashKey;
} // getNextStashKey


/**
 *  Unstash an object that has been previously stashed.
 *  * key - the key to a previously stashed object.
 */
void esp32_duktape_unstash_object(duk_context *ctx, uint32_t key) {
	duk_push_global_object(ctx);
	// [0] - Global object

	if (duk_get_prop_string(ctx, -1, CALLBACK_STASH_OBJECT_NAME) ==0) {
		// [0] - Global object
		// [1] - Undefined
		LOGE("esp32_duktape_unstash_object: No such object called \"%s\"", CALLBACK_STASH_OBJECT_NAME);
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

		LOGE("esp32_duktape_unstash_object: No such stash key: %d", key);

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
