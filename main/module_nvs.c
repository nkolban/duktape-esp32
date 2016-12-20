#include <duktape.h>
#include <nvs.h>

#include "esp32_specific.h"
#include "logging.h"
#include "module_nvs.h"

LOG_TAG("module_nvs");

/**
 * Open the NVS namespace
 * [0] - Namespace
 * [1] - open mode type "readwrite" or "readonly"
 *
 * Return:
 * [n] - Access handle
 */
static duk_ret_t js_nvs_open(duk_context *ctx) {
	const char *nameSpace = duk_get_string(ctx, -2);
	const char *openModeString = duk_get_string(ctx, -1);
	nvs_open_mode openMode;
	if (strcmp(openModeString, "readwrite") == 0) {
		openMode = NVS_READWRITE;
	} else if (strcmp(openModeString, "readonly") == 0) {
		openMode = NVS_READONLY;
	} else {
		LOGE("open mode not one of READWRITE or READONLY");
		return 0;
	}
	nvs_handle handle;
	nvs_open(nameSpace, openMode, &handle);
	duk_push_int(ctx, handle);
	return 1;
} // js_nvs_open


/*
 *  Close a previously opened handle.
 *  [0] - An NVS handle.
 *
 *  Returns:
 *  N/A
 */
static duk_ret_t js_nvs_close(duk_context *ctx) {
	nvs_handle handle = duk_get_int(ctx, -1);
	nvs_close(handle);
	return 0;
} // js_nvs_close


/*
 *  Commit changes to the NVS data store.
 *  [0] - An NVS handle.
 */
static duk_ret_t js_nvs_commit(duk_context *ctx) {
	nvs_handle handle = duk_get_int(ctx, -1);
	nvs_commit(handle);
	return 0;
} // js_nvs_commit


/*
 *  Get a value from the store.
 *  [0] - handle
 *  [1] - key
 *  [2] - type - one of:
 *  * str
 *  * blob
 *  * u32
 */
static duk_ret_t js_nvs_get(duk_context *ctx) {
	nvs_handle handle = duk_get_int(ctx, -3);
	const char *key = duk_get_string(ctx, -2);
	const char *type = duk_get_string(ctx, -1);
	//
	// u32
	//
	if (strcmp(type, "int") == 0) {
		uint32_t u32Val;
		esp_err_t rc = nvs_get_u32(handle, key, &u32Val);
		if (rc != ESP_OK) {
			duk_error(ctx, 1, "nvs_get_u32 rc=%s", esp32_errToString(rc));
		}
		LOGD("Getting NVS for %s = %d", key, u32Val);
		duk_push_int(ctx, u32Val);
	}
	//
	// blob
	//
	else if (strcmp(type, "buffer") == 0) {
		size_t length;
		esp_err_t rc = nvs_get_blob(handle, key, NULL, &length);
		if (rc != ESP_OK) {
			duk_error(ctx, 1, "nvs_get_blob rc=%s", esp32_errToString(rc));
		}
		// Allocate storage here ...

		void *data = duk_push_fixed_buffer(ctx, length);
		rc =nvs_get_blob(handle, key, data, &length);
		if (rc != ESP_OK) {
			duk_error(ctx, 1, "nvs_get_blob rc=%s", esp32_errToString(rc));
		}
		duk_push_buffer_object(ctx, -1, 0, length, DUK_BUFOBJ_NODEJS_BUFFER);
	}
	//
	// str
	//
	else if (strcmp(type, "string") == 0) {
		size_t length;
		esp_err_t rc = nvs_get_str(handle, key, NULL, &length);
		if (rc != ESP_OK) {
			duk_error(ctx, 1, "nvs_get_str rc=%s", esp32_errToString(rc));
		}
		char *data = malloc(length);
		rc = nvs_get_str(handle, key, data, &length);
		if (rc != ESP_OK) {
			duk_error(ctx, 1, "nvs_get_str rc=%s", esp32_errToString(rc));
		}
		duk_push_string(ctx, data);
		free(data);
	}
	//
	// unknown
	//
	else {
		LOGE("Unknown type: %s", type);
		return 0;
	}
	return 1;
} // js_nvs_get


/*
 *  Set a value from the store.
 *  [0] - handle
 *  [1] - key
 *  [2] - data
 *  [3] - type - one of:
 *  * str
 *  * blob
 *  * u32

 */
static duk_ret_t js_nvs_set(duk_context *ctx) {
	nvs_handle handle = duk_get_int(ctx, -4);
	const char *key = duk_get_string(ctx, -3);
	const char *type = duk_get_string(ctx, -1);

	//
	// u32
	//
	if (strcmp(type, "int") == 0) {
		uint32_t u32Val = duk_get_int(ctx, -2);
		LOGD("Setting nvs %s to %d", key, u32Val);
		esp_err_t rc = nvs_set_u32(handle, key, u32Val);
		if (rc != ESP_OK) {
			duk_error(ctx, 1, "nvs_set_u32 rc=%s", esp32_errToString(rc));
		}
	}
	//
	// blob
	//
	else if (strcmp(type, "buffer") == 0) {
		size_t length;
		const void *data = duk_get_buffer(ctx, -2, &length);
		esp_err_t rc = nvs_set_blob(handle, key, data, length);
		if (rc != ESP_OK) {
			duk_error(ctx, 1, "nvs_set_blob rc=%s", esp32_errToString(rc));
		}
	}
	//
	// str
	//
	else if (strcmp(type, "string") == 0) {
		const char *data = duk_get_string(ctx, -2);
		esp_err_t rc = nvs_set_str(handle, key, data);
		if (rc != ESP_OK) {
			duk_error(ctx, 1, "nvs_set_str rc=%s", esp32_errToString(rc));
		}
	}
	//
	// unknown
	//
	else {
		LOGE("Unknown type: %s", type);
		return 0;
	}
	return 0;
}


/**
 * Create the NVS module in Global.
 */
void ModuleNVS(duk_context *ctx) {
	duk_push_global_object(ctx);
	// [0] - Global object

	duk_idx_t idx = duk_push_object(ctx); // Create new NVS object
	// [0] - Global object
	// [1] - New object - NVS object

	duk_push_c_function(ctx, js_nvs_close, 1);
	// [0] - Global object
	// [1] - New object - NVS object
	// [2] - C Function - js_nvs_close

	duk_put_prop_string(ctx, idx, "close"); // Add close to new NVS
	// [0] - Global object
	// [1] - New object - NVS object

	duk_push_c_function(ctx, js_nvs_commit, 1);
	// [0] - Global object
	// [1] - New object - NVS object
	// [2] - C Function - js_nvs_commit

	duk_put_prop_string(ctx, idx, "commit"); // Add commit to new NVS
	// [0] - Global object
	// [1] - New object - NVS object

	duk_push_c_function(ctx, js_nvs_get, 3);
	// [0] - Global object
	// [1] - New object - NVS object
	// [2] - C Function - js_nvs_get

	duk_put_prop_string(ctx, idx, "get"); // Add get to new NVS
	// [0] - Global object
	// [1] - New object - NVS object

	duk_push_c_function(ctx, js_nvs_open, 2);
	// [0] - Global object
	// [1] - New object - NVS object
	// [2] - C Function - js_nvs_get

	duk_put_prop_string(ctx, idx, "open"); // Add open to new NVS
	// [0] - Global object
	// [1] - New object - NVS object

	duk_push_c_function(ctx, js_nvs_set, 4);
	// [0] - Global object
	// [1] - New object - NVS object
	// [2] - C Function - js_nvs_set

	duk_put_prop_string(ctx, idx, "set"); // Add set to new NVS
	// [0] - Global object
	// [1] - New object - NVS object

	duk_put_prop_string(ctx, 0, "_NVS"); // Add _NVS to global
	// [0] - Global object

	duk_pop(ctx);
	// <Empty Stack>
} // ModuleNVS
