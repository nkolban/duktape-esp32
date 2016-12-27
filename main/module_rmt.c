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


	duk_idx_t idx = duk_push_object(ctx);
	// [0] - Channel
	// [1] - New object


	duk_push_int(ctx, channel);
	// [0] - Channel
	// [1] - New object
	// [2] - Channel


	duk_put_prop_string(ctx, idx, "channel");
	// [0] - Channel
	// [1] - New object


	duk_push_boolean(ctx, loop_en);
	// [0] - Channel
	// [1] - New object
	// [2] - loopEnabled


	duk_put_prop_string(ctx, idx, "loopEnabled");
	// [0] - Channel
	// [1] - New object


	duk_push_int(ctx, div_cnt);
	// [0] - Channel
	// [1] - New object
	// [2] - clockDiv


	duk_put_prop_string(ctx, idx, "clockDiv");
	// [0] - Channel
	// [1] - New object


	duk_push_int(ctx, memNum);
	// [0] - Channel
	// [1] - New object
	// [2] - memBlocks


	duk_put_prop_string(ctx, idx, "memBlocks");
	// [0] - Channel
	// [1] - New object


	duk_push_int(ctx, 0); // FIX
	// [0] - Channel
	// [1] - New object
	// [2] - idleLevel


	duk_put_prop_string(ctx, idx, "idleLevel");
	// [0] - Channel
	// [1] - New object


	duk_push_string(ctx, owner==RMT_MEM_OWNER_TX?"TX":"RX");
	// [0] - Channel
	// [1] - New object
	// [2] - memoryOwner


	duk_put_prop_string(ctx, idx, "memoryOwner");
	// [0] - Channel
	// [1] - New object

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
	rmt_channel_t channel;
	duk_size_t length;
	duk_uarridx_t i;
	bool waitForWrite = 1;

	if (!duk_is_number(ctx, -2)) {
		LOGD("jms_rmt_write - param 1 is not a number")
		return 0;
	}
	channel = duk_get_int(ctx, -2); // Get the channel number from parameter 0.
	if (channel <0 || channel >= RMT_CHANNEL_MAX) {
		LOGD("jms_rmt_write - channel is out of range")
		return 0;
	}

	if (!duk_is_array(ctx, -1)) {
		LOGD("jms_rmt_write - param 2 is not an array")
		return 0;
	}
	length = duk_get_length(ctx, -1);
	/*
	duk_get_prop_string(ctx, 1, "length");
	length = duk_get_int(ctx, -1);
	duk_pop(ctx); // Pop the level
	*/

	LOGD("Length of RMT item array is %d", length);
	if (length == 0) {
		LOGD("jms_rmt_write - length of items is 0")
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
	// if 3 array -> 3 items [0, 1], [2, 3*]
	// if 4 array -> 4 items [0, 1], [2, 3], [4*, ?]
	// #of items array elements = (length div 2) + 2

	int itemArraySize = length / 2 + 2;

	rmt_item32_t *itemArray = calloc(sizeof(rmt_item32_t), itemArraySize);

	for (i=0; i<length; i++) {
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

		LOGD("item: %d - level=%d, duration=%d", i, level, duration);
	}

	setRMTItem(0, 0, length, itemArray); // Set the trailer / terminator.

	ESP_ERROR_CHECK(rmt_driver_install(
		channel, // Channel
		0, // RX ring buffer size
		19 // Interrupt number
	));
	ESP_ERROR_CHECK(rmt_write_items(channel, itemArray, itemArraySize, waitForWrite));
	ESP_ERROR_CHECK(rmt_driver_uninstall(channel));

	free(itemArray);
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
 *    clockDiv: [Optional]
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
		if (memBlocks == 0) {
			LOGE("memBlocks must be >= 1");
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
	config.tx_config.carrier_freq_hz = 1000;
	config.tx_config.carrier_level = RMT_CARRIER_LEVEL_LOW;
	config.tx_config.idle_output_en = 1;
	config.tx_config.idle_level = idleLevel;
	config.tx_config.loop_en = 0;

	ESP_ERROR_CHECK(rmt_config(&config));


	LOGD("<< js_rmt_txConfig");
  return 0;
} // js_fs_openSync



/**
 * Add native methods to the RMT object.
 * [0] - RMT Object
 */
duk_ret_t ModuleRMT(duk_context *ctx) {

	int idx = -2;
	duk_push_c_function(ctx, js_rmt_getState, 1);
	// [0] - RMT object
	// [1] - C Function - js_rmt_getState

	duk_put_prop_string(ctx, idx, "getState"); // Add getState to RMT
	// [0] - RMT object


	duk_push_c_function(ctx, js_rmt_txConfig, 2);
	// [0] - RMT object
	// [1] - C Function - js_rmt_txConfig

	duk_put_prop_string(ctx, idx, "txConfig"); // Add txConfig to RMT
	// [0] - RMT object


	duk_push_c_function(ctx, js_rmt_write, 2);
	// [0] - RMT object
	// [1] - C Function - js_rmt_write

	duk_put_prop_string(ctx, idx, "write"); // Add write to RMT
	// [0] - RMT object

	duk_pop(ctx);
	// <Empty Stack>
	return 0;
} // ModuleRMT
