#include <driver/i2c.h>
#include <duktape.h>

#include "duktape_utils.h"
#include "esp32_specific.h"
#include "logging.h"
#include "module_i2c.h"

LOG_TAG("module_i2c");


/*
 * [0] - options
 * o - port_num - The I2C port number we are using.  Either I2C_NUM_0 or I2C_NUM_1.
 * o - mode - The I2C mode.  Either I2C_MODE_MASTER or I2C_MODE_SLAVE.
 */
static duk_ret_t js_i2c_driver_install(duk_context *ctx) {
	LOGD(">> js_i2c_driver_install");
	i2c_mode_t mode;
	i2c_port_t port;
	if (duk_get_prop_string(ctx, -1, "port") != 1) {
		LOGE("No port supplied");
		return 0;
	}
	port = duk_get_int(ctx, -1);
	if (port != I2C_NUM_0 && port != I2C_NUM_1) {
		LOGE("Invalid port");
		return 0;
	}
	duk_pop(ctx);

	if (duk_get_prop_string(ctx, -1, "mode") != 1) {
		LOGE("No mode supplied");
		return 0;
	}
	mode = duk_get_int(ctx, -1);
	if (mode != I2C_MODE_MASTER && mode != I2C_MODE_SLAVE) {
		LOGE("Invalid mode");
		return 0;
	}
	duk_pop(ctx);

	esp_err_t errRc = i2c_driver_install(port, mode, 0, 0, 0);
	if (errRc != ESP_OK) {
		LOGE("i2c_driver_install: %s", esp32_errToString(errRc));
	}
	LOGD("<< js_i2c_driver_install");
	return 0;
} // js_i2c_driver_install


/*
 * Create a command handle.
 * Inputs:
 * N/A
 *
 * Return:
 * A JS Pointer
 */
static duk_ret_t js_i2c_cmd_link_create(duk_context *ctx) {
	LOGD(">> js_i2c_cmd_link_create");
	i2c_cmd_handle_t cmd_handle = i2c_cmd_link_create();
	duk_push_pointer(ctx, cmd_handle);
	LOGD("<< js_i2c_cmd_link_create");
	return 1;
} // js_i2c_cmd_link_create


/*
 * [0] - cmd_handle
 */
static duk_ret_t js_i2c_cmd_link_delete(duk_context *ctx) {
	LOGD(">> js_i2c_cmd_link_delete");
	i2c_cmd_handle_t cmd_handle = duk_get_pointer(ctx, -1);
	i2c_cmd_link_delete(cmd_handle);
	LOGD("<< js_i2c_cmd_link_delete");
	return 0;
} // js_i2c_cmd_link_delete


/*
 * [0] - i2c_num
 * [1] - cmd_handle
 * [2] - delay (msecs)
 *
 * return 0 on success and not 0 on error
 */
static duk_ret_t js_i2c_master_cmd_begin(duk_context *ctx) {
	LOGD(">> js_i2c_master_cmd_begin");
	i2c_port_t i2c_port = duk_get_int(ctx, -3);
	i2c_cmd_handle_t cmd_handle = duk_get_pointer(ctx, -2);
	int delay = duk_get_int(ctx, -1);
	esp_err_t errRc = i2c_master_cmd_begin(i2c_port, cmd_handle, delay/portTICK_PERIOD_MS);
	if (errRc != ESP_OK) {
		LOGE("Error: i2c_master_cmd_begin: esp_err_t=%s", esp32_errToString(errRc));
	}
	LOGD("<< js_i2c_master_cmd_begin: %d", errRc);
	duk_push_int(ctx, errRc);
	return 1;
} // js_i2c_master_cmd_begin


/*
 * [0] - js pointer- cmd_handle
 */
static duk_ret_t js_i2c_master_start(duk_context *ctx) {
	LOGD(">> js_i2c_master_start");
	i2c_cmd_handle_t cmd_handle = duk_get_pointer(ctx, -1);
	esp_err_t errRc = i2c_master_start(cmd_handle);
	if (errRc != ESP_OK) {
		LOGE("i2c_master_start: %s", esp32_errToString(errRc));
	}
	LOGD("<< js_i2c_master_start");
	return 0;
} // js_i2c_master_start

/*
 * [0] - js pointer- cmd_handle
 */
static duk_ret_t js_i2c_master_stop(duk_context *ctx) {
	LOGD(">> js_i2c_master_stop");
	i2c_cmd_handle_t cmd_handle = duk_get_pointer(ctx, -1);
	esp_err_t errRc = i2c_master_stop(cmd_handle);
	if (errRc != ESP_OK) {
		LOGE("Error: i2c_master_stop: esp_err_t=%s", esp32_errToString(errRc));
	}
	LOGD("<< js_i2c_master_stop");
	return 0;
} // js_i2c_master_stop


/*
 * [0] - js pointer- cmd_handle
 * [1] - buffer - buffer to write
 * [2] - boolean - look for an Ack
 */
static duk_ret_t js_i2c_master_write(duk_context *ctx) {
	LOGD(">> js_i2c_master_write");
	size_t data_len;
	uint8_t *data;
	i2c_cmd_handle_t cmd_handle = duk_get_pointer(ctx, -3);
	data = duk_get_buffer_data(ctx, -2, &data_len);
	bool ack_en = duk_get_boolean(ctx, -1);

	esp_err_t errRc = i2c_master_write(cmd_handle, data, data_len, ack_en);
	if (errRc != ESP_OK) {
		LOGE("Error: i2c_master_write: esp_err_t=%s", esp32_errToString(errRc));
	}
	LOGD("<< js_i2c_master_write");
	return 0;
} // js_i2c_master_write

/*
 * [0] - js pointer- cmd_handle
 * [1] - number - byte to write
 * [2] - boolean - look for an Ack
 */
static duk_ret_t js_i2c_master_write_byte(duk_context *ctx) {



	i2c_cmd_handle_t cmd_handle = duk_get_pointer(ctx, -3);
	uint8_t data = (uint8_t)duk_get_int(ctx, -2);
	bool ack_en = duk_get_boolean(ctx, -1);
	LOGD(">> js_i2c_master_write_byte: data=0x%2x, ack=%d", data, ack_en);

	if (!duk_is_pointer(ctx, -3) || !duk_is_number(ctx, -2) || !duk_is_boolean(ctx, -1)) {
		LOGE("Invalid parameters");
		return 0;
	}

	esp_err_t errRc = i2c_master_write_byte(cmd_handle, data, ack_en);
	if (errRc != ESP_OK) {
		LOGE("Error: i2c_master_write_byte: esp_err_t=%s", esp32_errToString(errRc));
	}
	LOGD("<< js_i2c_master_write_byte");
	return 0;
} // js_i2c_master_write_byte


/*
 * [0] - options
 * o port_num
 * o mode
 * o sda_pin
 * o scl_pin
 * o master_clk_speed
 */
static duk_ret_t js_i2c_param_config(duk_context *ctx) {

	LOGD(">> js_i2c_param_config");
	i2c_mode_t mode;
	i2c_port_t port;
	gpio_num_t sda_pin;
	gpio_num_t scl_pin;
	uint32_t master_clk_speed;

	if (!duk_is_object(ctx, -1)) {
		LOGE("Parameter not supplied");
		return 0;
	}

	// port
	if (duk_get_prop_string(ctx, -1, "port") != 1) {
		LOGE("No port");
		return 0;
	}
	port = duk_get_int(ctx, -1);
	if (port != I2C_NUM_0 && port != I2C_NUM_1) {
		LOGE("Invalid port");
		return 0;
	}
	duk_pop(ctx);

	// mode
	if (duk_get_prop_string(ctx, -1, "mode") != 1) {
		LOGE("No mode");
		return 0;
	}
	mode = duk_get_int(ctx, -1);
	if (mode != I2C_MODE_MASTER && mode != I2C_MODE_SLAVE) {
		LOGE("Invalid mode");
		return 0;
	}
	duk_pop(ctx);

	// sda_pin
	if (duk_get_prop_string(ctx, -1, "sda_pin") != 1) {
		LOGE("No sda_pin");
		return 0;
	}
	sda_pin = duk_get_int(ctx, -1);
	duk_pop(ctx);

	// scl_pin
	if (duk_get_prop_string(ctx, -1, "scl_pin") != 1) {
		LOGE("No scl_pin");
		return 0;
	}
	scl_pin = duk_get_int(ctx, -1);
	duk_pop(ctx);

	if (mode == I2C_MODE_MASTER) {
		if (duk_get_prop_string(ctx, -1, "master_clk_speed") != 1) {
			master_clk_speed = 100000; // Default value
		} else {
			master_clk_speed = duk_get_int(ctx, -1);
		}
		duk_pop(ctx);
	}

	if (mode == I2C_MODE_MASTER) {
		LOGD("mode=I2C_MODE_MASTER, port_num=%d, sda_pin=%d, scl_pin=%d, master_clk_speed=%d",
			port, sda_pin, scl_pin, master_clk_speed);
	} else {
		LOGD("mode=I2C_MODE_SLAVE, port_num=%d, sda_pin=%d, scl_pin=%d",
			port, sda_pin, scl_pin);
	}

	i2c_config_t conf;
	conf.mode = mode;
	conf.sda_io_num = sda_pin;
	conf.scl_io_num = scl_pin;
	conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
	conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
	if (mode == I2C_MODE_MASTER) {
		conf.master.clk_speed = master_clk_speed;
	}

	esp_err_t errRc = i2c_param_config(port, &conf);
	if (errRc != ESP_OK) {
		LOGE("Error: i2c_param_config: esp_err_t=%s", esp32_errToString(errRc));
	}
	LOGD("<< js_i2c_param_config");
	return 0;
} // js_i2c_param_config


/**
 * Add native methods to the I2C object.
 * [0] - I2C Object
 */
duk_ret_t ModuleI2C(duk_context *ctx) {
	ADD_FUNCTION("cmd_link_create",   js_i2c_cmd_link_create,   0);
	ADD_FUNCTION("cmd_link_delete",   js_i2c_cmd_link_delete,   0);
	ADD_FUNCTION("driver_install",    js_i2c_driver_install,    1);
	ADD_FUNCTION("master_cmd_begin",  js_i2c_master_cmd_begin,  3);
	ADD_FUNCTION("master_start",      js_i2c_master_start,      1);
	ADD_FUNCTION("master_stop",       js_i2c_master_stop,       1);
	ADD_FUNCTION("master_write",      js_i2c_master_write,      3);
	ADD_FUNCTION("master_write_byte", js_i2c_master_write_byte, 3);
	ADD_FUNCTION("param_config",      js_i2c_param_config,      1);

	ADD_INT("I2C_NUM_0",        I2C_NUM_0);
	ADD_INT("I2C_NUM_1",        I2C_NUM_1);
	ADD_INT("I2C_MODE_MASTER",  I2C_MODE_MASTER);
	ADD_INT("I2C_MODE_SLAVE",   I2C_MODE_SLAVE);
	ADD_INT("I2C_MASTER_READ",  I2C_MASTER_READ);
	ADD_INT("I2C_MASTER_WRITE", I2C_MASTER_WRITE);

	duk_pop(ctx);
	// <Empty Stack>
	return 0;
} // ModuleI2C
