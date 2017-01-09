/**
 * RMT processing.
 * The RMT is the Remote Control Trasmitter (a strange name) but it controls
 * pulse width at high speed.
 *
 * The module is permanent at RMT.
 * It has the following methods:
 *
 * * getState
 * * txConfig
 */

#include <driver/rmt.h>
#include <duktape.h>
#include <errno.h>
#include <esp_log.h>
#include <esp_system.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

#include "duktape_utils.h"
#include "esp32_specific.h"
#include "logging.h"
#include "sdkconfig.h"

LOG_TAG("module_rmt");

/**
 * Get the state of a channel.
 * [0] - number - The channel id.
 * Returns:
 * {
 * 		channel: <number> - The channel id.
 *    loopEnabled: <boolean> - Whether or not the TX loop is enabled.
 *    clockDiv: <number> - The clock divisor.
 *    memBlocks: <number> - The memory blocks being used.
 *    idleLevel: <number> - The idle level.
 *    memoryOwner: <String> - The memory owner "TX" or "RX"
 * }
 */
static duk_ret_t js_rmt_getState(duk_context *ctx) {
	bool loop_en;
	uint8_t div_cnt;
	uint8_t memNum;
	bool lowPowerMode;
	rmt_mem_owner_t owner;
	uint16_t idleThreshold;
	uint32_t status;
	rmt_source_clk_t srcClk;
	rmt_channel_t channel;

	channel = duk_get_int(ctx, -1); // Get the channel number from stack top.

	rmt_get_tx_loop_mode(channel, &loop_en);
	rmt_get_clk_div(channel, &div_cnt);
	rmt_get_mem_block_num(channel, &memNum);
	rmt_get_mem_pd(channel, &lowPowerMode);
	rmt_get_memory_owner(channel, &owner);
	rmt_get_rx_idle_thresh(channel, &idleThreshold);
	rmt_get_status(channel, &status);
	rmt_get_source_clk(channel, &srcClk);

	duk_push_object(ctx);
	// [0] - Channel
	// [1] - New object

	ADD_INT("channel", channel);
	ADD_BOOLEAN("loopEnabled", loop_en);
	ADD_INT("clockDiv", div_cnt);
	ADD_INT("memBlocks", memNum);
	ADD_INT("idleLevel", 0); // FIX
	ADD_STRING("memoryOwner", owner==RMT_MEM_OWNER_TX?"TX":"RX");

	return 1;
} // js_rmt_getState


/**
 * Set the item value.
 */
static void setRMTItem(
		uint8_t level,
		uint16_t duration,
		int item,
		rmt_item32_t *itemArray) {
	LOGD("setRMTItem: item: %2d, duration: %d, level: %d", item, duration, level);
	rmt_item32_t *ptr = &itemArray[item/2];
	if (item%2 == 0) {
		ptr->duration0 = duration;
		ptr->level0 = level;
	} else {
		ptr->duration1 = duration;
		ptr->level1 = level;
	}
} // setItem


/**
 * Write items into the output stream.  This call will block
 * until the transmission is complete.
 * [0] - channel
 * [1] - array of items.  Each item is an object of the form:
 * {
 *    level: <boolean> - The output signal level.
 *    duration: <number> - The duration of this signal.
 * }
 */
static duk_ret_t js_rmt_write(duk_context *ctx) {
	LOGD(">> js_rmt_write");
	rmt_channel_t channel;
	duk_size_t dataItemCount;
	duk_uarridx_t i;
	bool waitForWrite = 1;

	if (!duk_is_number(ctx, -2)) {
		LOGE("<< js_rmt_write: param 1 is not a number")
		return 0;
	}
	channel = duk_get_int(ctx, -2); // Get the channel number from parameter 0.
	if (channel <0 || channel >= RMT_CHANNEL_MAX) {
		LOGE("<< jms_rmt_write - channel is out of range")
		return 0;
	}

	if (!duk_is_array(ctx, -1)) {
		LOGE("<< jms_rmt_write - param 2 is not an array")
		return 0;
	}
	dataItemCount = duk_get_length(ctx, -1);
	/*
	duk_get_prop_string(ctx, 1, "length");
	length = duk_get_int(ctx, -1);
	duk_pop(ctx); // Pop the level
	*/

	LOGD("Length of RMT data item array is %d", dataItemCount);
	if (dataItemCount == 0) {
		LOGD("<< jms_rmt_write - length of items is 0")
		return 0;
	}

	// We are working with an array of objects where each object contains:
	// - level: <boolean> - signal level
	// - duration: <number> - duration of level in RMT ticks

	// An RMT_item is composed of 4 fields:
	// * duration0
	// * level0
	// * duration1
	// * level1
	//
	// The last entry must have a duration of 0 which means we allocate an extra entry.
	// The number of items we need is length + 1
	// If 1 array -> 2 items [0, 1*]
	// If 2 array -> 3 items [0, 1], [2*, ?]
	// if 3 array -> 4 items [0, 1], [2, 3*]
	// if 4 array -> 5 items [0, 1], [2, 3], [4*, ?]
	//
	// Now let us think of the rmt_item32_t.  This is a 32 bit value that holds TWO data items.
	// | Number of data items | number of rmt_item32_t |
	// +----------------------+------------------------+
	// | 2                    | 1                      |
	// | 3                    | 2                      |
	// | 4                    | 2                      |
	// | 5                    | 3                      |
	//
	// Proposition:
	// The number of rmt_item32_t elements to host n data items is
	// (n+1) div 2
	// n -> value
	// 2 -> 1
	// 3 -> 2
	// 4 -> 2
	// 5 -> 3

	dataItemCount++; // Add 1 for the null terminator.

	int itemArraySize = (dataItemCount + 1) / 2;

	rmt_item32_t *itemArray = calloc(sizeof(rmt_item32_t), itemArraySize);
	bzero(itemArray, sizeof(rmt_item32_t) * itemArraySize);

	for (i=0; i<dataItemCount-1; i++) {
		// Get each of the items and work with it.
		duk_get_prop_index(ctx, -1, i);

		duk_get_prop_string(ctx, -1, "level");
		uint8_t level = duk_get_boolean(ctx, -1);
		duk_pop(ctx); // Pop the level

		duk_get_prop_string(ctx, -1, "duration");
		uint16_t duration = duk_get_int(ctx, -1);
		duk_pop(ctx); // Pop the duration

		duk_pop(ctx); // Pop the item object

		setRMTItem(level, duration, i, itemArray);

		//LOGD("item: %2d - level=%d, duration=%d", i, level, duration);
	}

	setRMTItem(0, 0, dataItemCount-1, itemArray); // Set the trailer / terminator.

	ESP_ERROR_CHECK(rmt_write_items(channel, itemArray, itemArraySize, waitForWrite));

	free(itemArray);
	LOGD("<< js_rmt_write");
	return 0;
} // js_rmt_write


/**
 * Configure a channel.
 * Two parameters
 * 1) Channel id
 * 2) Configuration:
 * {
 * 		gpio: <number> - GPIO pin to use.
 * 		memBlocks: <number> - Memory blocks to use. Optional, default is 1.
 * 		idleLevel: <boolean> - Idle value to use.  Optional, default is LOW.
 * 		clockDiv: <number> - Clock divider.  Optional, default is 1.
 * }
 *
 * [0] - number - Channel
 * [1] - object - Configuration
 * {
 *    gpio:
 *    memBlocks: [Optional]
 *    idleLevel: [Optional]
 *    clockDiv:  [Optional]
 * }
 *
 */
static duk_ret_t js_rmt_txConfig(duk_context *ctx) {
	LOGD(">> js_rmt_txConfig");
	rmt_channel_t channel;
	gpio_num_t gpio;
	uint8_t memBlocks = 1;
	rmt_idle_level_t idleLevel = RMT_IDLE_LEVEL_LOW;
	uint8_t clockDiv = 1;

	channel = duk_get_int(ctx, 0);
	if (channel >= RMT_CHANNEL_MAX) {
		LOGE("Channel out of range");
		return 0;
	}

	if (duk_get_prop_string(ctx, 1, "gpio")) {
		gpio = duk_get_int(ctx, -1);
	} else {
		LOGE("No mandatory gpio supplied");
		return 0;
	}
	duk_pop(ctx);

	if (duk_get_prop_string(ctx, 1, "memBlocks")) {
		memBlocks = duk_get_int(ctx, -1);
		if (memBlocks < 1 || memBlocks > 8) {
			LOGE("memBlocks must be >= 1 and <=8");
			return 0;
		}
	}
	duk_pop(ctx);

	if (duk_get_prop_string(ctx, 1, "idleLevel")) {
		if (duk_get_boolean(ctx, -1)) {
			idleLevel = RMT_IDLE_LEVEL_HIGH;
		} else {
			idleLevel = RMT_IDLE_LEVEL_LOW;
		}
	}
	duk_pop(ctx);

	if (duk_get_prop_string(ctx, 1, "clockDiv")) {
		clockDiv = duk_get_int(ctx, -1);
		if (clockDiv == 0) {
			LOGE("clockDiv must be >= 1");
		}
	}
	duk_pop(ctx);

	rmt_config_t config;
	config.channel = channel;
	config.clk_div = clockDiv;
	config.gpio_num = gpio;
	config.mem_block_num = memBlocks;
	config.rmt_mode = RMT_MODE_TX;
	config.tx_config.carrier_duty_percent = 50;
	config.tx_config.carrier_en = 0;
	config.tx_config.carrier_freq_hz = 10000;
	config.tx_config.carrier_level = RMT_CARRIER_LEVEL_HIGH;
	config.tx_config.idle_output_en = 1;
	config.tx_config.idle_level = idleLevel;
	config.tx_config.loop_en = 0;

/*
	rmt_config_t config;
	config.rmt_mode = RMT_MODE_TX;
	config.channel = channel;
	config.gpio_num = gpio;
	config.mem_block_num = 1;
	config.tx_config.loop_en = 0;
	config.tx_config.carrier_en = 0;
	config.tx_config.idle_output_en = 1;
	config.tx_config.idle_level = (rmt_idle_level_t)0;
	config.tx_config.carrier_duty_percent = 50;
	config.tx_config.carrier_freq_hz = 10000;
	config.tx_config.carrier_level = (rmt_carrier_level_t)1;
	config.clk_div = 8;
	*/

	ESP_ERROR_CHECK(rmt_config(&config));
	esp_err_t errCode = rmt_driver_install(
		channel, // Channel
		0, // RX ring buffer size
		0 // Interrupt flags
	);
	if (errCode != ESP_OK) {
		LOGE("rmt_driver_install: %s", esp32_errToString(errCode));
	}

	LOGD("<< js_rmt_txConfig");
  return 0;
} // js_fs_openSync


/**
 * Add native methods to the RMT object.
 * [0] - RMT Object
 */
duk_ret_t ModuleRMT(duk_context *ctx) {

	ADD_FUNCTION("getState", js_rmt_getState, 1);
	ADD_FUNCTION("txConfig", js_rmt_txConfig, 2);
	ADD_FUNCTION("write",    js_rmt_write,    2);

	return 0;
} // ModuleRMT
