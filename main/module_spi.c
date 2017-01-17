#include <driver/spi_master.h>
#include <duktape.h>

#include "duktape_utils.h"
#include "esp32_specific.h"
#include "logging.h"
#include "module_spi.h"

LOG_TAG("module_spi");

#define DEFAULT_HOST HSPI_HOST
#define DEFAULT_CLOCK_SPEED (10000)


/*
 * [0] - options
 * * host - optional - Default to HSPI_HOST
 * * mode - optional -
 * * clock_speed - optional -
 *
 * Return:
 * handle
 */
static duk_ret_t js_spi_bus_add_device(duk_context *ctx) {
	LOGD(">> js_spi_bus_add_device");
	spi_host_device_t host = DEFAULT_HOST;
	spi_device_interface_config_t dev_config;
	spi_device_handle_t *handle;

	dev_config.command_bits     = 0;
	dev_config.address_bits     = 0;
	dev_config.dummy_bits       = 0;
	dev_config.duty_cycle_pos   = 0;
	dev_config.cs_ena_posttrans = 0;
	dev_config.cs_ena_pretrans  = 0;
	dev_config.clock_speed_hz   = DEFAULT_CLOCK_SPEED;
	dev_config.spics_io_num     = -1;
	dev_config.flags            = 0;
	dev_config.queue_size       = 10;
	dev_config.pre_cb           = NULL;
	dev_config.post_cb          = NULL;
	dev_config.mode             = 0;

	if (!duk_is_object(ctx, -1)) {
		LOGE("<< js_spi_bus_add_device: No options object.");
		return 0;
	}

	// host
	if (duk_get_prop_string(ctx, -1, "host") == 1) {
		host = duk_get_int(ctx, -1);
	}
	duk_pop(ctx);

	// mode
	if (duk_get_prop_string(ctx, -1, "mode") == 1) {
		dev_config.mode = duk_get_int(ctx, -1);
	}
	duk_pop(ctx);

	// clock_speed
	if (duk_get_prop_string(ctx, -1, "clock_speed") == 1) {
		dev_config.clock_speed_hz = duk_get_int(ctx, -1);
	}
	duk_pop(ctx);

	// cs
	if (duk_get_prop_string(ctx, -1, "cs") == 1) {
		dev_config.spics_io_num = duk_get_int(ctx, -1);
	}
	duk_pop(ctx);

	handle = malloc(sizeof(spi_device_handle_t));
	assert(handle != NULL);

	LOGD(" - host: %d, mode: %d, spics_io_num:%d, clock_speed_hz: %d",
		host, dev_config.mode, dev_config.spics_io_num, dev_config.clock_speed_hz);

	esp_err_t errRc = spi_bus_add_device(
		host,
	  &dev_config,
	  handle);
	if (errRc != ESP_OK) {
		LOGE("<< js_spi_bus_add_device: %s", esp32_errToString(errRc));
		free(handle);
		return 0;
	}

	duk_push_pointer(ctx, handle);
	LOGD("<< js_spi_bus_add_device");
	return 1;
} // js_spi_bus_add_device


/*
 * [0] - host
 */
static duk_ret_t js_spi_bus_free(duk_context *ctx) {
	LOGD(">> js_spi_bus_free");
	spi_host_device_t host = DEFAULT_HOST;
	if (duk_is_number(ctx, -1)) {
		host = duk_get_int(ctx, -1);
	}
	esp_err_t errRc = spi_bus_free(host);
	if (errRc != ESP_OK) {
		LOGE("<< js_spi_bus_free: %s", esp32_errToString(errRc));
		return 0;
	}
	LOGD("<< js_spi_bus_free");
	return 0;
} // js_spi_bus_free


/*
 * Initialize the SPI environment identifying the pins that are to be used for
 * distinct tasks.
 *
 * [0] - Options
 * * host - Optional - Default to HSPI_HOST
 * * mosi -
 * * miso
 * * clk
 */
static duk_ret_t js_spi_bus_initialize(duk_context *ctx) {
	LOGD(">> js_spi_bus_initialize");
	spi_host_device_t host = DEFAULT_HOST;
	spi_bus_config_t bus_config;
	int dma_chan;

	bus_config.mosi_io_num   = -1; // MOSI
	bus_config.miso_io_num   = -1; // MISO
	bus_config.sclk_io_num = -1; // CLK
	bus_config.quadwp_io_num  = -1;
	bus_config.quadhd_io_num  = -1;
	dma_chan = 1;

	if (!duk_is_object(ctx, -1)) {
		LOGE("<< js_spi_bus_initialize: No options object.");
		return 0;
	}

	// mosi
	if (duk_get_prop_string(ctx, -1, "mosi") == 1) {
		bus_config.mosi_io_num = duk_get_int(ctx, -1);
	}
	duk_pop(ctx);

	// miso
	if (duk_get_prop_string(ctx, -1, "miso") == 1) {
		bus_config.miso_io_num = duk_get_int(ctx, -1);
	}
	duk_pop(ctx);

	// clk
	if (duk_get_prop_string(ctx, -1, "clk") == 1) {
		bus_config.sclk_io_num = duk_get_int(ctx, -1);
	}
	duk_pop(ctx);

	// host
	if (duk_get_prop_string(ctx, -1, "host") == 1) {
		host = duk_get_int(ctx, -1);
	}
	duk_pop(ctx);

	if (bus_config.sclk_io_num == -1) {
		LOGE("<< js_spi_bus_initialize: No clk pin supplied");
		return 0;
	}

	if (bus_config.mosi_io_num == -1 && bus_config.miso_io_num == -1) {
		LOGE("<< js_spi_bus_initialize: Neither mosi nor miso pins supplied");
		return 0;
	}

	LOGD("- mosi=%d, miso=%d, clk=%d, host=%d",
		bus_config.mosi_io_num,
		bus_config.miso_io_num,
		bus_config.sclk_io_num,
		host);

	esp_err_t errRc = spi_bus_initialize(host, &bus_config, dma_chan);
	if (errRc != ESP_OK) {
		LOGE("<< js_spi_bus_initialize: %s", esp32_errToString(errRc));
		return 0;
	}
	LOGD("<< js_spi_bus_initialize");
	return 0;
} // js_spi_bus_initialize


/*
 * [0] - handle
 */
static duk_ret_t js_spi_bus_remove_device(duk_context *ctx) {
	LOGD(">> js_spi_bus_remove_device");

	if (!duk_is_pointer(ctx, -1)) {
		LOGE("<< js_spi_bus_remove_device: Invalid handle");
		return 0;
	}

	spi_device_handle_t *handle = duk_get_pointer(ctx, -1);
	esp_err_t errRc = spi_bus_remove_device(*handle);
	free(handle);
	if (errRc != ESP_OK) {
		LOGE("<< spi_bus_remove_device: %s", esp32_errToString(errRc));
		return 0;
	}
	LOGD("<< js_spi_bus_remove_device");
	return 0;
} // js_spi_bus_remove_device


/*
 * [0] - handle
 * [1] - data (buffer)
 */
static duk_ret_t js_spi_device_transmit(duk_context *ctx) {
	LOGD(">> js_spi_device_transmit");
	if (!duk_is_pointer(ctx, -2)) {
		LOGE("<< js_spi_device_transmit: Invalid handle");
		return 0;
	}
	spi_device_handle_t *handle = duk_get_pointer(ctx, -2);

	if (!duk_is_buffer_data(ctx, -1)) {
		LOGE("<< js_spi_device_transmit: Invalid data");
		return 0;
	}

	size_t size;
	void *data = duk_get_buffer_data(ctx, -1, &size);

	spi_transaction_t trans_desc;
	trans_desc.flags     = 0;
	trans_desc.command   = 0;
	trans_desc.address   = 0;
	trans_desc.length    = size * 8; // The length property is size in bits.
	trans_desc.rxlength  = 0;
	trans_desc.user      = NULL;
	trans_desc.tx_buffer = data;
	trans_desc.rx_buffer = data;

	LOGD(" - Transmitting %d bits of data.", trans_desc.length);
	esp_err_t errRc = spi_device_transmit(*handle, &trans_desc);
	if (errRc != ESP_OK) {
		LOGE("<< js_spi_device_transmit: %s", esp32_errToString(errRc));
		return 0;
	}

	LOGD("<< js_spi_device_transmit");
	return 0;
} // js_spi_device_transmit


/**
 * Add native methods to the SPI object.
 * [0] - SPI Object
 */
duk_ret_t ModuleSPI(duk_context *ctx) {

	ADD_FUNCTION("add_device",     js_spi_bus_add_device,    1);
	ADD_FUNCTION("bus_free",       js_spi_bus_free,          1);
	ADD_FUNCTION("bus_initialize", js_spi_bus_initialize,    1);
	ADD_FUNCTION("remove_device",  js_spi_bus_remove_device, 1);
	ADD_FUNCTION("transmit",       js_spi_device_transmit,   2);

	ADD_INT("SPI_HOST",  SPI_HOST);
	ADD_INT("HSPI_HOST", HSPI_HOST);
	ADD_INT("VSPI_HOST", VSPI_HOST);

	return 0;
} // ModuleSPI
