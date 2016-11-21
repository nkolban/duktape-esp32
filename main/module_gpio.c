#include <stdbool.h>
#include <esp_log.h>
#include <esp_system.h>
#include <duktape.h>
/**
 * Implement the GPIO functions.
 *
 * The file provides the implementation of the GPIO module which
 * provides:
 * * digitalRead
 * * digitalWrite
 * * getPinMode
 * * pinMode
 *
 */
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <driver/gpio.h>
#include "duktape_utils.h"
#include "sdkconfig.h"

static char tag[] = "module_gpio";


static duk_ret_t js_gpio_getPinMode(duk_context *ctx) {
	ESP_LOGD(tag, ">> js_gpio_getPinMode");
	ESP_LOGD(tag, "Not implemented");
	ESP_LOGD(tag, "<< js_gpio_getPinMode");
	return 0;
} // js_gpio_getPinMode


/**
 * Read a pin.
 * [0] - Integer - pin number
 */
static duk_ret_t js_gpio_digitalRead(duk_context *ctx) {
	ESP_LOGD(tag, ">> js_gpio_digitalRead");
	int pinNumber = duk_get_int(ctx, 0);
	if (pinNumber < 0 || pinNumber >39) {
		ESP_LOGD(tag, "Pin out of range");
		return 0;
	}
	duk_push_boolean(ctx, gpio_get_level(pinNumber));
	ESP_LOGD(tag, "<< js_gpio_digitalRead");
	return 1;
} // js_gpio_digitalRead


/**
 * Write to a pin.
 * [0] - Integer - pin number
 * [1] - boolean - value
 */
static duk_ret_t js_gpio_digitalWrite(duk_context *ctx) {
	ESP_LOGD(tag, ">> js_gpio_digitalWrite");
	int pinNumber = duk_get_int(ctx, 0);
	int value = duk_get_boolean(ctx, 1);
	if (pinNumber < 0 || pinNumber >39) {
		ESP_LOGD(tag, "Pin out of range");
		return 0;
	}
	gpio_set_level(pinNumber, value);
	ESP_LOGD(tag, "<< js_gpio_digitalWrite");
	return 0;
} // js_gpio_digitalWrite


/**
 * Set the mode of a pin.
 * [0] - Integer - pin number.
 * [1] - String - mode - analog, input, output
 */
static duk_ret_t js_gpio_pinMode(duk_context *ctx) {
	ESP_LOGD(tag, ">> js_gpio_pinMode");
	int pinNumber = duk_get_int(ctx, 0);
	const char *modeString = duk_get_string(ctx, 1);
	gpio_mode_t mode;
	if (strcmp(modeString, "analog") == 0) {
		mode = GPIO_MODE_INPUT;
	} else if (strcmp(modeString, "input") == 0) {
		mode = GPIO_MODE_INPUT;
	} if (strcmp(modeString, "output") == 0) {
		mode = GPIO_MODE_OUTPUT;
	} else {
		ESP_LOGD(tag, "Unknown mode: %s", modeString);
		return 0;
	}
	if (pinNumber < 0 || pinNumber >39) {
		ESP_LOGD(tag, "Pin out of range");
		return 0;
	}
	gpio_pad_select_gpio(pinNumber); // Ask the ESP32 to use the specified pin for GPIO.
	gpio_set_direction(pinNumber, mode); // Set the GPIO mode.
	ESP_LOGD(tag, "<< js_gpio_pinMode");
	return 0;
} // js_gpio_pinMode



/**
 * Create the GPIO module in Global.
 */
void ModuleGPIO(duk_context *ctx) {
	duk_push_global_object(ctx);
	duk_push_object(ctx); // Create new FS object

	duk_push_c_function(ctx, js_gpio_getPinMode, 1);
	duk_put_prop_string(ctx, -2, "getPinMode"); // Add getPinMode to new GPIO

	duk_push_c_function(ctx, js_gpio_digitalRead, 1);
	duk_put_prop_string(ctx, -2, "digitalRead"); // Add digitalRead to new GPIO

	duk_push_c_function(ctx, js_gpio_digitalWrite, 2);
	duk_put_prop_string(ctx, -2, "digitalWrite"); // Add digitalWrite to new GPIO

	duk_push_c_function(ctx, js_gpio_pinMode, 2);
	duk_put_prop_string(ctx, -2, "pinMode"); // Add pinMode to new GPIO

	duk_put_prop_string(ctx, -2, "GPIO"); // Add GPIO to global
} // ModuleGPIO
