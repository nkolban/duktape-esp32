#include <driver/adc.h>
#include <duktape.h>

#include "duktape_utils.h"
#include "esp32_specific.h"
#include "logging.h"
#include "module_adc.h"

LOG_TAG("module_adc");

/*
 * [0] - ADC channel
 * [1] - Attenuation
 *
 * Return:
 * N/A
 */
static duk_ret_t js_adc_config_channel_atten(duk_context *ctx) {
	adc1_channel_t channel = duk_get_int(ctx, -2);
	adc_atten_t atten = duk_get_int(ctx, -1);
	LOGD(">> js_adc_config_channel_atten: channel=%d, atten=%d", channel, atten);
	esp_err_t errRc = adc1_config_channel_atten(channel, atten);
	if (errRc != ESP_OK) {
		LOGE("Error: adc1_config_channel_atten: esp_err_t=%s", esp32_errToString(errRc));
	}
	LOGD("<< js_adc_config_channel_atten");
	return 0;
} // js_adc_config_channel_atten


/*
 * [0] - resolution
 *
 * Return:
 * N/A
 */
static duk_ret_t js_adc_config_width(duk_context *ctx) {
	adc_bits_width_t resolution = duk_get_int(ctx, -1);
	LOGD(">> js_adc_config_width: resolution=%d", resolution);
	esp_err_t errRc = adc1_config_width(resolution);
	if (errRc != ESP_OK) {
		LOGE("Error: adc1_config_width: esp_err_t=%s", esp32_errToString(errRc));
	}
	LOGD("<< js_adc_config_width");
	return 0;
} // js_adc_config_width


/*
 * [0] - ADC channel
 *
 * Return:
 * Number - value
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

	ADD_FUNCTION("config_channel_atten", js_adc_config_channel_atten, 2);
	ADD_FUNCTION("config_width",         js_adc_config_width,         1);
	ADD_FUNCTION("get_voltage",          js_adc_get_voltage,          1);

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

	return 0;
} // ModuleADC
