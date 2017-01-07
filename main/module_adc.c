#include <driver/adc.h>
#include <duktape.h>

#include "duktape_utils.h"
#include "esp32_specific.h"
#include "logging.h"
#include "module_adc.h"

LOG_TAG("module_adc");

/*
 * [0] - ADC channel
 */
static duk_ret_t js_adc_get_voltage(duk_context *ctx) {
	LOGD(">> js_adc_get_voltage");
	adc1_channel_t channel = duk_get_int(ctx, -1);
	int value = adc1_get_voltage(channel);
	duk_push_int(ctx, value);
	LOGD("<< js_adc_get_voltage");
	return 1;
} // js_adc_get_voltage


/**
 * Add native methods to the ADC object.
 * [0] - ADC Object
 */
duk_ret_t ModuleADC(duk_context *ctx) {

	ADD_FUNCTION("get_voltage", js_adc_get_voltage, 0);

	ADD_INT("ADC_WIDTH_9BIT",  ADC_WIDTH_9Bit);
	ADD_INT("ADC_WIDTH_10BIT", ADC_WIDTH_10Bit);
	ADD_INT("ADC_WIDTH_11BIT", ADC_WIDTH_11Bit);
	ADD_INT("ADC_WIDTH_12BIT", ADC_WIDTH_12Bit);
	ADD_INT("ADC_ATTEN_0DB",   ADC_ATTEN_0db);
	ADD_INT("ADC_ATTEN_2_5DB", ADC_ATTEN_2_5db);
	ADD_INT("ADC_ATTEN_6DB",   ADC_ATTEN_6db);
	ADD_INT("ADC_ATTEN_11DB",  ADC_ATTEN_11db);
	ADD_INT("ADC_CHANNEL_0",   ADC1_CHANNEL_0);
	ADD_INT("ADC_CHANNEL_1",   ADC1_CHANNEL_1);
	ADD_INT("ADC_CHANNEL_2",   ADC1_CHANNEL_2);
	ADD_INT("ADC_CHANNEL_3",   ADC1_CHANNEL_3);
	ADD_INT("ADC_CHANNEL_4",   ADC1_CHANNEL_4);
	ADD_INT("ADC_CHANNEL_5",   ADC1_CHANNEL_5);
	ADD_INT("ADC_CHANNEL_6",   ADC1_CHANNEL_6);
	ADD_INT("ADC_CHANNEL_7",   ADC1_CHANNEL_7);

	duk_pop(ctx);
	// <Empty Stack>
	return 0;
} // ModuleADC
