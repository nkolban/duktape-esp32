#include <driver/i2c.h>
#include <duktape.h>
#include "logging.h"
#include "module_i2c.h"

LOG_TAG("i2c");


/*
 * [0] - options
 * o - port_num
 * o - mode
 */
static duk_ret_t js_i2c_driver_install(duk_context *ctx) {
	LOGD(">> js_i2c_driver_install");
	i2c_mode_t mode;
	i2c_port_t port;
	duk_get_prop_string(ctx, -1, "port_num");
	port = duk_get_int(ctx, -1);
	duk_pop(ctx);

	duk_get_prop_string(ctx, -1, "mode");
	mode = duk_get_int(ctx, -1);
	duk_pop(ctx);

	i2c_driver_install(port, mode, 0, 0, 0);
	LOGD("<< js_i2c_driver_install");
	return 0;
} // js_i2c_driver_install

/*
 * [0] - options
 * o port_num
 * o mode
 * o sda_pin
 * o scl_pin
 * o sda_pullup
 * o scl_pullup
 * o master_clk_speed
 */
static duk_ret_t js_i2c_param_config(duk_context *ctx) {

	LOGD(">> js_i2c_param_config");
	i2c_mode_t mode;
	i2c_port_t port;
	gpio_num_t sda_pin;
	gpio_num_t scl_pin;
	uint32_t master_clk_speed;

	duk_get_prop_string(ctx, -1, "port_num");
	port = duk_get_int(ctx, -1);
	duk_pop(ctx);

	duk_get_prop_string(ctx, -1, "mode");
	mode = duk_get_int(ctx, -1);
	duk_pop(ctx);

	duk_get_prop_string(ctx, -1, "sda_pin");
	sda_pin = duk_get_int(ctx, -1);
	duk_pop(ctx);

	duk_get_prop_string(ctx, -1, "scl_pin");
	scl_pin = duk_get_int(ctx, -1);
	duk_pop(ctx);

	if (mode == I2C_MODE_MASTER) {
		duk_get_prop_string(ctx, -1, "master_clk_speed");
		master_clk_speed = duk_get_int(ctx, -1);
		duk_pop(ctx);
	}

	if (mode == I2C_MODE_MASTER) {
		LOGD(" mode=I2C_MODE_MASTER, port_num=%d, sda_pin=%d, scl_pin=%d, master_clk_speed=%d",
			port, sda_pin, scl_pin, master_clk_speed);
	} else {
		LOGD(" mode=I2C_MODE_SLAVE, port_num=%d, sda_pin=%d, scl_pin=%d",
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

	i2c_param_config(port, &conf);
	LOGD("<< js_i2c_param_config");
	return 0;
} // js_i2c_param_config


/**
 * Add native methods to the I2C object.
 * [0] - I2C Object
 */
duk_ret_t ModuleI2C(duk_context *ctx) {

	int idx = -2;
	duk_push_c_function(ctx, js_i2c_driver_install, 1);
	// [0] - I2C object
	// [1] - C Function - js_i2c_driver_install

	duk_put_prop_string(ctx, idx, "driver_install"); // Add driver_install to I2C
	// [0] - I2C object

	duk_push_c_function(ctx, js_i2c_param_config, 1);
	// [0] - I2C object
	// [1] - C Function - js_i2c_param_config

	duk_put_prop_string(ctx, idx, "param_config"); // Add param_config to I2C
	// [0] - I2C object


	duk_pop(ctx);
	// <Empty Stack>
	return 0;
} // ModuleI2C
