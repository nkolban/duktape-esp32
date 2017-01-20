#include <duktape.h>
#include <nvs.h>

#include "esp32_specific.h"
#include "duktape_utils.h"
#include "logging.h"
#include "module_nvs.h"

LOG_TAG("module_nvs");

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
	esp_err_t rc = nvs_commit(handle);
	if (rc != ESP_OK) {
		duk_error(ctx, 1, "nvs_commit rc=%s", esp32_errToString(rc));
	}
	return 0;
} // js_nvs_commit


/*
 * Erase a specific key.
 * [0] - An NVS handle.
 * [1] - key
 */
static duk_ret_t js_nvs_erase(duk_context *ctx) {
	if (!duk_is_string(ctx, -1)) {
		LOGE(">> js_nvs_erase: No key supplied");
		return 0;
	}
	if (!duk_is_number(ctx, -2)) {
		LOGE(">> js_nvs_erase: Invalid handle supplied");
		return 0;
	}
	const char *key = duk_get_string(ctx, -1);
	nvs_handle handle = duk_get_int(ctx, -2);
	esp_err_t rc = nvs_erase_key(handle, key);
	if (rc != ESP_OK) {
		duk_error(ctx, 1, "nvs_key_all key=%s, rc=%s", key, esp32_errToString(rc));
	}
	return 0;
} // js_nvs_erase


/*
 * Erase all the name/value pairs for this handle.
 * [0] - An NVS handle.
 */
static duk_ret_t js_nvs_eraseAll(duk_context *ctx) {
	nvs_handle handle = duk_get_int(ctx, -1);
	esp_err_t rc = nvs_erase_all(handle);
	if (rc != ESP_OK) {
		duk_error(ctx, 1, "nvs_erase_all rc=%s", esp32_errToString(rc));
	}
	return 0;
} // js_nvs_eraseAll


/*
 *  Get a value from the store.  If no such value is found, then
 *  null is returned.
 *
 *  [0] - handle
 *  [1] - key
 *  [2] - type - one of:
 *  * string
 *  * buffer
 *  * int
 *  * uint8
 */
static duk_ret_t js_nvs_get(duk_context *ctx) {
	nvs_handle handle = duk_get_int(ctx, -3);
	const char *key   = duk_get_string(ctx, -2);
	const char *type  = duk_get_string(ctx, -1);

	//
	// int
	//
	if (strcmp(type, "int") == 0) {
		uint32_t u32Val;
		esp_err_t rc = nvs_get_u32(handle, key, &u32Val);
		if (rc == ESP_ERR_NVS_NOT_FOUND) {
			goto notFound;
		}
		if (rc != ESP_OK) {
			duk_error(ctx, 1, "nvs_get_u32 rc=%s", esp32_errToString(rc));
		}
		LOGD("Getting NVS for %s = %d", key, u32Val);
		duk_push_int(ctx, u32Val);
	}

	//
	// uint8
	//
	if (strcmp(type, "uint8") == 0) {
		uint8_t u8Val;
		esp_err_t rc = nvs_get_u8(handle, key, &u8Val);
		if (rc == ESP_ERR_NVS_NOT_FOUND) {
			goto notFound;
		}
		if (rc != ESP_OK) {
			duk_error(ctx, 1, "nvs_get_u8 rc=%s", esp32_errToString(rc));
		}
		LOGD("Getting NVS for %s = %d", key, u8Val);
		duk_push_int(ctx, u8Val);
	}


	//
	// buffer
	//
	else if (strcmp(type, "buffer") == 0) {
		size_t length;
		esp_err_t rc = nvs_get_blob(handle, key, NULL, &length);
		if (rc == ESP_ERR_NVS_NOT_FOUND) {
			goto notFound;
		}
		if (rc != ESP_OK) {
			duk_error(ctx, 1, "nvs_get_blob rc=%s", esp32_errToString(rc));
		}
		// Allocate storage here ...

		void *data = duk_push_fixed_buffer(ctx, length);
		rc = nvs_get_blob(handle, key, data, &length);
		if (rc == ESP_ERR_NVS_NOT_FOUND) {
			goto notFound;
		}
		if (rc != ESP_OK) {
			duk_error(ctx, 1, "nvs_get_blob rc=%s", esp32_errToString(rc));
		}
		duk_push_buffer_object(ctx, -1, 0, length, DUK_BUFOBJ_NODEJS_BUFFER);
	}

	//
	// string
	//
	else if (strcmp(type, "string") == 0) {
		size_t length;
		esp_err_t rc = nvs_get_str(handle, key, NULL, &length);
		if (rc == ESP_ERR_NVS_NOT_FOUND) {
			goto notFound;
		}
		if (rc != ESP_OK) {
			duk_error(ctx, 1, "nvs_get_str rc=%s", esp32_errToString(rc));
		}
		char *data = malloc(length);
		rc = nvs_get_str(handle, key, data, &length);
		if (rc == ESP_ERR_NVS_NOT_FOUND) {
			goto notFound;
		}
		if (rc != ESP_OK) {
			duk_error(ctx, 1, "nvs_get_str rc=%s", esp32_errToString(rc));
		}
		LOGD("Getting NVS for %s = %s", key, data);
		duk_push_string(ctx, data);
		free(data);
	}
	//
	// unknown
	//
	else {
		LOGE("Unknown type: %s", type);
		goto notFound;
	}
	return 1;

notFound:
	duk_push_null(ctx);
	return 1;
} // js_nvs_get


/**
 * Open the NVS namespace
 * [0] - Namespace
 * [1] - open mode type "readwrite" or "readonly"
 *
 * Return:
 * [n] - Access handle
 */
static duk_ret_t js_nvs_open(duk_context *ctx) {
	const char *nameSpace      = duk_get_string(ctx, -2);
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
	esp_err_t errRc = nvs_open(nameSpace, openMode, &handle);
	if (errRc != ESP_OK) {
		LOGE("Error: nvs_open: %s", esp32_errToString(errRc));
		duk_push_null(ctx);
		return 1;
	}
	duk_push_int(ctx, handle);
	return 1;
} // js_nvs_open


/*
 *  Set a value from the store.
 *  [0] - handle
 *  [1] - key
 *  [2] - data
 *  [3] - type - one of:
 *  * string
 *  * buffer
 *  * blob
 *  * int
 *  * uint8
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
	// u8
	//
	if (strcmp(type, "uint8") == 0) {
		uint8_t u8Val = (uint8_t)duk_get_int(ctx, -2);
		LOGD("Setting nvs %s to %d", key, u8Val);
		esp_err_t rc = nvs_set_u8(handle, key, u8Val);
		if (rc != ESP_OK) {
			duk_error(ctx, 1, "nvs_set_u8 rc=%s", esp32_errToString(rc));
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
} //js_nvs_set


/**
 * Create the NVS module in Global.
 */
duk_ret_t ModuleNVS(duk_context *ctx) {

	ADD_FUNCTION("close",    js_nvs_close,    1);
	ADD_FUNCTION("commit",   js_nvs_commit,   1);
	ADD_FUNCTION("erase",    js_nvs_erase,    2);
	ADD_FUNCTION("eraseAll", js_nvs_eraseAll, 1);
	ADD_FUNCTION("get",      js_nvs_get,      3);
	ADD_FUNCTION("open",     js_nvs_open,     2);
	ADD_FUNCTION("set",      js_nvs_set,      4);

	return 0;
} // ModuleNVS
