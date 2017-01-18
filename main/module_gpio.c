/*
 * Encapsulate the ESP32 GPIO functions.  The functions exposed
 * are:
 * gpioGetLevel
 * gpioISRHandlerAdd
 * gpioISRHandlerRemove
 * gpioInit
 * gpioInstallISRService
 * gpioSetDirection
 * gpioSetIntrType
 * gpioSetLevel
 * gpioSetPullMode
 */

#include <driver/gpio.h>
#include <duktape.h>
#include <esp_log.h>

#include "duktape_event.h"
#include "duktape_utils.h"
#include "esp32_specific.h"
#include "logging.h"
#include "module_gpio.h"
#include "sdkconfig.h"

static uint32_t g_gpioISRHandlerStashKey = -1;

LOG_TAG("module_gpio");

static int gpio_isr_handler_dataProvider(duk_context *ctx, void *context) {
	gpio_num_t pin = (gpio_num_t)context;
	duk_push_int(ctx, pin);
	return 1;
}

/*
 * GPIO ISR handler
 * This function will be called when a GPIO interrupt occurs.
 */
static void gpio_isr_handler(void *args) {
	event_newCallbackRequestedEvent(
		ESP32_DUKTAPE_CALLBACK_TYPE_ISR_FUNCTION,
		g_gpioISRHandlerStashKey,
		gpio_isr_handler_dataProvider,
		args
	);
} // gpio_isr_handler

/*
 * Get the GPIO level of the pin.
 * [0] - Pin number
 */
static duk_ret_t js_os_gpioGetLevel(duk_context *ctx) {
	gpio_num_t pinNum = duk_get_int(ctx, -1);
	if (pinNum < 0 || pinNum >= GPIO_NUM_MAX) {
		LOGE("js_os_gpioGetLevel: Pin %d out of range", pinNum);
		return 0;
	}
	int level = gpio_get_level(pinNum);
	if (level == 0) {
		duk_push_false(ctx);
	} else {
		duk_push_true(ctx);
	}
	return 1;
} // js_os_gpioGetLevel

/*
 * Initialize the GPIO pin.
 * [0] - Pin number
 */
static duk_ret_t js_os_gpioInit(duk_context *ctx) {
	gpio_num_t pinNum = duk_get_int(ctx, -1);
	if (pinNum < 0 || pinNum >= GPIO_NUM_MAX) {
		LOGE("js_os_gpioInit: Pin %d out of range", pinNum);
		return 0;
	}
	gpio_pad_select_gpio(pinNum);
	return 0;
} // js_os_gpioInit


/*
 * [0] - Int - flags
 * [1] - function - handler function
 */
static duk_ret_t js_os_gpioInstallISRService(duk_context *ctx) {
	int flags = duk_get_int(ctx, -2);
	if (!duk_is_function(ctx, -1)) {
		LOGD("js_os_gpioInstallISRService: not a function!");
		return 0;
	}

	esp_err_t errRc = gpio_install_isr_service(flags);
	if (errRc != ESP_OK) {
		LOGE("gpio_install_isr_service: %s", esp32_errToString(errRc));
	}

	g_gpioISRHandlerStashKey = esp32_duktape_stash_array(ctx, 1);
	return 0;
} // js_os_gpioInstallISRService


// [0] - pin
static duk_ret_t js_os_gpioISRHandlerAdd(duk_context *ctx) {
	int pinNum = duk_get_int(ctx, -1);
	if (pinNum < 0 || pinNum >= GPIO_NUM_MAX) {
		LOGE("js_os_gpioISRHandlerAdd: Pin %d out of range", pinNum);
		return 0;
	}
	esp_err_t errRc = gpio_isr_handler_add(pinNum, gpio_isr_handler, (void *)pinNum);
	if (errRc != ESP_OK) {
		LOGE("gpio_isr_handler_add: %s", esp32_errToString(errRc));
	}
	return 0;
} // js_os_gpioISRHandlerAdd


// [0] - pin
static duk_ret_t js_os_gpioISRHandlerRemove(duk_context *ctx) {
	int pin = duk_get_int(ctx, -1);
	esp_err_t errRc = gpio_isr_handler_remove(pin);
	if (errRc != ESP_OK) {
		LOGE("gpio_isr_handler_remove: %s", esp32_errToString(errRc));
	}
	return 0;
}


/*
 * Set the GPIO direction of the pin.
 * [0] - Pin number
 * [1] - Direction - 0=Input, 1=output
 */
static duk_ret_t js_os_gpioSetDirection(duk_context *ctx) {
	gpio_mode_t mode;
	gpio_num_t pinNum = duk_get_int(ctx, -2);
	if (pinNum < 0 || pinNum >= GPIO_NUM_MAX) {
		LOGE("js_os_gpioSetDirection: Pin %d out of range", pinNum);
		return 0;
	}
	int modeVal = duk_get_int(ctx, -1);
	if (modeVal == 1) {
		mode = GPIO_MODE_OUTPUT;
	} else {
		mode = GPIO_MODE_INPUT;
	}
	esp_err_t rc = gpio_set_direction(pinNum, mode);
	if (rc != 0) {
		LOGE("gpio_set_direction: %s", esp32_errToString(rc));
	}
	return 0;
} // js_os_gpioSetDirection


/*
 * Set the interrupt type that the pin will respond to.
 * [0] - pin
 * [1] - Interrupt type - Choices are:
 *  - GPIO_INTR_ANYEDGE
 *  - GPIO_INTR_DISABLE
 *  - GPIO_INTR_NEGEDGE
 *  - GPIO_INTR_POSEDGE
 */
static duk_ret_t js_os_gpioSetIntrType(duk_context *ctx) {
	gpio_num_t pinNum = (gpio_num_t)duk_get_int(ctx, -2);
	if (pinNum < 0 || pinNum >= GPIO_NUM_MAX) {
		LOGE("js_os_gpioSetIntrType: Pin %d out of range", pinNum);
		return 0;
	}
	gpio_int_type_t type = (gpio_int_type_t)duk_get_int(ctx, -1);
	if (type < 0 || type >= GPIO_INTR_MAX) {
		LOGE("js_os_gpioSetIntrType: Interrupt type %d out of range", type);
		return 0;
	}

	esp_err_t errRc = gpio_set_intr_type(pinNum, type);
	if (errRc != 0) {
		LOGE("gpio_set_intr_type: %s", esp32_errToString(errRc));
	}
	return 0;
} // js_os_gpioSetIntrType


/*
 * Set the GPIO level of the pin.
 * [0] - Pin number
 * [1] - level - true or false
 */
static duk_ret_t js_os_gpioSetLevel(duk_context *ctx) {
	uint32_t level;
	gpio_num_t pinNum = duk_get_int(ctx, -2);
	if (pinNum < 0 || pinNum >= GPIO_NUM_MAX) {
		LOGE("js_os_gpioSetLevel: Pin %d out of range", pinNum);
		return 0;
	}
	duk_bool_t levelBool = duk_get_boolean(ctx, -1);
	if (levelBool == 0) {
		level = 0;
	} else {
		level = 1;
	}
	esp_err_t errRc = gpio_set_level(pinNum, level);
	if (errRc != 0) {
		LOGE("gpio_set_level: %s", esp32_errToString(errRc));
	}
	return 0;
} // js_os_gpioSetLevel


/*
 * Set the GPIO level of the pin.
 * [0] - Pin number
 * [1] - mode - pull mode.  One of:
 *  - GPIO_PULLUP_ONLY
 *  - GPIO_PULLDOWN_ONLY
 *  - GPIO_PULLUP_PULLDOWN
 *  - GPIO_FLOATING
 */
static duk_ret_t js_os_gpioSetPullMode(duk_context *ctx) {
	gpio_num_t pinNum = duk_get_int(ctx, -2);
	if (pinNum < 0 || pinNum >= GPIO_NUM_MAX) {
		LOGE("js_os_gpioSetPullMode: Pin %d out of range", pinNum);
		return 0;
	}
	gpio_pull_mode_t pullMode = duk_get_int(ctx, -1);
	esp_err_t errRc = gpio_set_pull_mode(pinNum, pullMode);
	if (errRc != ESP_OK) {
		LOGE("gpio_set_pull_mode: %s", esp32_errToString(errRc));
	}
	return 0;
} // js_os_gpioSetPullMode


/**
 * Add native methods to the GPIO object.
 * [0] - GPIO Object
 */
duk_ret_t ModuleGPIO(duk_context *ctx) {

	ADD_FUNCTION("gpioGetLevel",          js_os_gpioGetLevel,          1);
	ADD_FUNCTION("gpioISRHandlerAdd",     js_os_gpioISRHandlerAdd,     1);
	ADD_FUNCTION("gpioISRHandlerRemove",  js_os_gpioISRHandlerRemove,  1);
	ADD_FUNCTION("gpioInit",              js_os_gpioInit,              1);
	ADD_FUNCTION("gpioInstallISRService", js_os_gpioInstallISRService, 2);
	ADD_FUNCTION("gpioSetDirection",      js_os_gpioSetDirection,      2);
	ADD_FUNCTION("gpioSetIntrType",       js_os_gpioSetIntrType,       2);
	ADD_FUNCTION("gpioSetLevel",          js_os_gpioSetLevel,          2);
	ADD_FUNCTION("gpioSetPullMode",       js_os_gpioSetPullMode,       2);

	ADD_INT("INTR_ANYEDGE",    GPIO_INTR_ANYEDGE);
	ADD_INT("INTR_DISABLE",    GPIO_INTR_DISABLE);
	ADD_INT("INTR_NEGEDGE",    GPIO_INTR_NEGEDGE);
	ADD_INT("INTR_POSEDGE",    GPIO_INTR_POSEDGE);
	ADD_INT("PULLUP_ONLY",     GPIO_PULLUP_ONLY);
	ADD_INT("PULLDOWN_ONLY",   GPIO_PULLDOWN_ONLY);
	ADD_INT("PULLUP_PULLDOWN", GPIO_PULLUP_PULLDOWN);
	ADD_INT("FLOATING",        GPIO_FLOATING);
	return 0;
} // ModuleGPIO
