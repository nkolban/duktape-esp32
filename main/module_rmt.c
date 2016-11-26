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

#include <stdbool.h>
#include <esp_log.h>
#include <esp_system.h>
#include <duktape.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <driver/rmt.h>
#include "duktape_utils.h"
#include "sdkconfig.h"

static char tag[] = "module_rmt";

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

	channel = duk_get_int(ctx, 0); // Get the channel number from parameter 0.

	rmt_get_tx_loop_mode(channel, &loop_en);
	rmt_get_clk_div(channel, &div_cnt);
	rmt_get_mem_block_num(channel, &memNum);
	rmt_get_mem_pd(channel, &lowPowerMode);
	rmt_get_memory_owner(channel, &owner);
	rmt_get_rx_idle_thresh(channel, &idleThreshold);
	rmt_get_status(channel, &status);
	rmt_get_source_clk(channel, &srcClk);

	duk_idx_t idx = duk_push_object(ctx);

	duk_push_int(ctx, channel);
	duk_put_prop_string(ctx, idx, "channel");

	duk_push_boolean(ctx, loop_en);
	duk_put_prop_string(ctx, idx, "loopEnabled");

	duk_push_int(ctx, div_cnt);
	duk_put_prop_string(ctx, idx, "clockDiv");

	duk_push_int(ctx, memNum);
	duk_put_prop_string(ctx, idx, "memBlocks");

	duk_push_int(ctx, 0); // FIX
	duk_put_prop_string(ctx, idx, "idleLevel");

	duk_push_string(ctx, owner==RMT_MEM_OWNER_TX?"TX":"RX");
	duk_put_prop_string(ctx, idx, "memoryOwner");
	return 1;
} // js_rmt_getState


/**
 * Configure a channel.
 * Two parameters
 * 1) Channel id
 * 2) Configuration:
 * {
 * 		gpio: <number> - GPIO pin to use.
 * 		memBlocks: <number> - Memory blocks to use.
 * 		idleLevel: <boolean> - Idle value to use.
 * 		clockDiv: <number> - Clock divider.
 * }
 * [0] - number - Channel
 * [1] - object - Configuration
 */
static duk_ret_t js_rmt_txConfig(duk_context *ctx) {
	ESP_LOGD(tag, ">> js_rmt_txConfig");
	rmt_channel_t channel;
	gpio_num_t gpio;
	uint8_t memBlocks = 1;
	rmt_idle_level_t idleLevel = RMT_IDLE_LEVEL_LOW;
	uint8_t clockDiv = 1;

	channel = duk_get_int(ctx, 0);
	if (channel >= RMT_CHANNEL_MAX) {
		ESP_LOGE(tag, "Channel out of range");
		return 0;
	}

	if (duk_get_prop_string(ctx, 1, "gpio")) {
		gpio = duk_get_int(ctx, -1);
	} else {
		ESP_LOGE(tag, "No mandatory gpio supplied");
		return 0;
	}
	duk_pop(ctx);

	if (duk_get_prop_string(ctx, 1, "memBlocks")) {
		memBlocks = duk_get_int(ctx, -1);
		if (memBlocks == 0) {
			ESP_LOGE(tag, "memBlocks must be >= 1");
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
			ESP_LOGE(tag, "clockDiv must be >= 1");
		}
	}
	duk_pop(ctx);

	rmt_config_t config;
	config.channel = channel;
	config.rmt_mode = RMT_MODE_TX;
	config.clk_div = clockDiv;
	config.gpio_num = gpio;
	config.mem_block_num = memBlocks;
	config.tx_config.loop_en = 0;
	config.tx_config.carrier_en = 0;
	config.tx_config.carrier_freq_hz = 1000;
	config.tx_config.carrier_duty_percent = 50;
	config.tx_config.carrier_level = RMT_CARRIER_LEVEL_LOW;
	config.tx_config.idle_output_en = 1;
	config.tx_config.idle_level = idleLevel;
	ESP_ERROR_CHECK(rmt_config(&config));

	ESP_LOGD(tag, "<< js_rmt_txConfig");
  return 1;
} // js_fs_openSync



/**
 * Create the RMT module in Global.
 */
void ModuleRMT(duk_context *ctx) {
	duk_push_global_object(ctx);
	duk_idx_t idx = duk_push_object(ctx); // Create new RMT object

	duk_push_c_function(ctx, js_rmt_txConfig, 2);
	duk_put_prop_string(ctx, idx, "txConfig"); // Add txConfig to new RS

	duk_push_c_function(ctx, js_rmt_getState, 1);
	duk_put_prop_string(ctx, idx, "getState"); // Add getState to new RS

	duk_put_prop_string(ctx, 0, "RMT"); // Add RMT to global
} // ModuleFS
