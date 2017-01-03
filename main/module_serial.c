#include <driver/uart.h>
#include <duktape.h>

#include "duktape_utils.h"
#include "esp32_specific.h"
#include "logging.h"
#include "module_serial.h"

#define UART_INTR_NUM 17

LOG_TAG("module_serial");

/*
 * Configure the serial interface.
 * [0] - Serial port to configure.
 * [1] - options
 * {
 *    baud: <baud rate> - default 115200
 *    rxBufferSize: Size of RX Buffer must be > UART_FIFO_LEN - default UART_FIFO_LEN + 1
 *    txBufferSize: Size of TX Buffer - default 0.
 *    rxPin: pin number for RX - default No change.
 *    txPin: pun number for TX - default No change.
 * }
 */
static duk_ret_t js_serial_configure(duk_context *ctx) {
	int port;
	int baud;
	int rx_buffer_size;
	int tx_buffer_size;
	int tx_pin = UART_PIN_NO_CHANGE;
	int rx_pin = UART_PIN_NO_CHANGE;
	esp_err_t errRc;

	port = duk_get_int(ctx, -2);
	if (port < 0 || port > 2) {
		LOGE("js_serial_configure: Invalid port number");
		return 0;
	}

	// Get the baud rate
	if (duk_get_prop_string(ctx, -1, "baud") != 1) {
		duk_pop(ctx);
		baud = 115200;
	} else {
		baud = duk_get_int(ctx, -1);
		duk_pop(ctx);
	}

	// Get the rx buffer size
	if (duk_get_prop_string(ctx, -1, "rxBufferSize") != 1) {
		duk_pop(ctx);
		rx_buffer_size = UART_FIFO_LEN + 1;
	} else {
		rx_buffer_size = duk_get_int(ctx, -1);
		duk_pop(ctx);
		if (rx_buffer_size <= UART_FIFO_LEN) {
			rx_buffer_size = UART_FIFO_LEN + 1;
		}
	}

	// Get the tx buffer size
	if (duk_get_prop_string(ctx, -1, "txBufferSize") != 1) {
		duk_pop(ctx);
		tx_buffer_size = 0;
	} else {
		tx_buffer_size = duk_get_int(ctx, -1);
		duk_pop(ctx);
	}

	if (duk_get_prop_string(ctx, -1, "rxPin") == 1) {
		rx_pin = duk_get_int(ctx, -1);
		duk_pop(ctx);
	} else {
		LOGD("No rxPin found");
		duk_pop(ctx);
	}

	if (duk_get_prop_string(ctx, -1, "txPin") == 1) {
		tx_pin = duk_get_int(ctx, -1);
		duk_pop(ctx);
	} else {
		LOGD("No txPin found");
		duk_pop(ctx);
	}

	uart_config_t myUartConfig;
	myUartConfig.baud_rate = baud;
	myUartConfig.data_bits = UART_DATA_8_BITS;
	myUartConfig.parity = UART_PARITY_DISABLE;
	myUartConfig.stop_bits = UART_STOP_BITS_1;
	myUartConfig.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
	myUartConfig.rx_flow_ctrl_thresh = 120;

	LOGD("Setting UART %d to baud %d", port, baud);

	errRc = uart_param_config(port, &myUartConfig);
	if (errRc != ESP_OK) {
		LOGE("uart_param_config: %s", esp32_errToString(errRc));
		return 0;
	}

	LOGD("Setting port %d pins to tx=%d, rx=%d", port, tx_pin, rx_pin);
	errRc = uart_set_pin(port, tx_pin, rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
	if (errRc != ESP_OK) {
		LOGE("uart_set_pin: %s", esp32_errToString(errRc));
		return 0;
	}

	LOGD("Setting port %d rxBufferSize: %d, txBufferSize: %d", port, rx_buffer_size, tx_buffer_size);
	errRc = uart_driver_install(
		port, // Port
		rx_buffer_size, // RX buffer size
		tx_buffer_size, // TX buffer size
		10, // queue size
		NULL, // Queue
		0 // Interrupt allocation flags
	);
	if (errRc != ESP_OK) {
		LOGE("uart_driver_install: %s", esp32_errToString(errRc));
		return 0;
	}
	return 0;
} // js_serial_configure


/*
 * Read data through the serial interface.
 * [0] - The serial port to read from.
 * [1] - Buffer into which to store data.
 *
 * Returns the length actually read.
 */
static duk_ret_t js_serial_read(duk_context *ctx) {
	size_t size;
	esp_err_t errRc;
	int port = duk_get_int(ctx, -2);
	if (port < 0 || port > 2) {
		LOGE("js_serial_write: Invalid port number");
		return 0;
	}
	void *data = duk_get_buffer_data(ctx, -1, &size);
	if (data == NULL) {
		return 0;
	}

	size_t available;
	errRc = uart_get_buffered_data_len(port, &available);
	if (errRc != ESP_OK) {
		LOGE("uart_get_buffered_data_len: %s", esp32_errToString(errRc));
		duk_push_int(ctx, 0);
		return 1;
	}
	if (available == 0) {
		duk_push_int(ctx, 0);
		return 1;
	}
	if (available > size) {
		available = size;
	}

	int numRead = uart_read_bytes(port, data, available, 0);
	duk_push_int(ctx, numRead);
	return 1;
} // js_serial_read


/*
 * Write data through the serial interface.
 * [0] - Serial port to write.
 * [1] - data to write (string or buffer).
 */
static duk_ret_t js_serial_write(duk_context *ctx) {
	size_t size;
	int port = duk_get_int(ctx, -2);
	if (port < 0 || port > 2) {
		LOGE("js_serial_write: Invalid port number");
		return 0;
	}
	const void *data = esp32_duktape_dataFromStringOrBuffer(ctx, -1, &size);
	if (data == NULL || size == 0) {
		return 0;
	}
	LOGD("Writing %d bytes to serial port %d", size, port);
	uart_write_bytes(port, data, size);
	return 0;
} // js_serial_write

/**
 * Add native methods to the Serial object.
 * [0] - Serial Object
 */
duk_ret_t ModuleSerial(duk_context *ctx) {

	int idx = -2;
	duk_push_c_function(ctx, js_serial_configure, 2);
	// [0] - Serial object
	// [1] - C Function - js_serial_configure

	duk_put_prop_string(ctx, idx, "configure"); // Add configure to Serial
	// [0] - Serial object

	duk_push_c_function(ctx, js_serial_read, 2);
	// [0] - Serial object
	// [1] - C Function - js_serial_read

	duk_put_prop_string(ctx, idx, "read"); // Add read to Serial
	// [0] - Serial object

	duk_push_c_function(ctx, js_serial_write, 2);
	// [0] - Serial object
	// [1] - C Function - js_serial_write

	duk_put_prop_string(ctx, idx, "write"); // Add write to Serial
	// [0] - Serial object


	duk_pop(ctx);
	// <Empty Stack>
	return 0;
} // ModuleSerial
