#include <esp_log.h>
#include <esp_partition.h>

#include "duktape.h"
#include "duktape_utils.h"
#include "logging.h"
#include "module_partitions.h"
#include "sdkconfig.h"

LOG_TAG("module_partition");

static const char *partitionSubtypeToString(esp_partition_subtype_t subtype) {
	switch(subtype) {
	case ESP_PARTITION_SUBTYPE_APP_FACTORY:
		return "APP_FACTORY";
	case ESP_PARTITION_SUBTYPE_APP_OTA_0:
		return "APP_OTA_0";
	case ESP_PARTITION_SUBTYPE_APP_OTA_1:
		return "APP_OTA_1";
	case ESP_PARTITION_SUBTYPE_APP_OTA_10:
		return "APP_OTA_10";
	case ESP_PARTITION_SUBTYPE_APP_OTA_11:
		return "APP_OTA_11";
	case ESP_PARTITION_SUBTYPE_APP_OTA_12:
		return "APP_OTA_12";
	case ESP_PARTITION_SUBTYPE_APP_OTA_13:
		return "APP_OTA_13";
	case ESP_PARTITION_SUBTYPE_APP_OTA_14:
		return "APP_OTA_14";
	case ESP_PARTITION_SUBTYPE_APP_OTA_15:
		return "APP_OTA_15";
	case ESP_PARTITION_SUBTYPE_APP_OTA_2:
		return "APP_OTA_2";
	case ESP_PARTITION_SUBTYPE_APP_OTA_3:
		return "APP_OTA_3";
	case ESP_PARTITION_SUBTYPE_APP_OTA_4:
		return "APP_OTA_4";
	case ESP_PARTITION_SUBTYPE_APP_OTA_5:
		return "APP_OTA_5";
	case ESP_PARTITION_SUBTYPE_APP_OTA_6:
		return "APP_OTA_6";
	case ESP_PARTITION_SUBTYPE_APP_OTA_7:
		return "APP_OTA_7";
	case ESP_PARTITION_SUBTYPE_APP_OTA_8:
		return "APP_OTA_8";
	case ESP_PARTITION_SUBTYPE_APP_OTA_9:
		return "APP_OTA_9";
	case ESP_PARTITION_SUBTYPE_APP_TEST:
		return "APP_TEST";
	case ESP_PARTITION_SUBTYPE_DATA_ESPHTTPD:
		return "DATA_ESPHTTPD";
	case ESP_PARTITION_SUBTYPE_DATA_FAT:
		return "DATA_FAT";
	case ESP_PARTITION_SUBTYPE_DATA_NVS:
		return "DATA_NVS";
	case ESP_PARTITION_SUBTYPE_DATA_PHY:
		return "DATA_PHY";
	case ESP_PARTITION_SUBTYPE_DATA_SPIFFS:
		return "DATA_SPIFFS";
	default:
		return "Unknown";
	}
} // partitionSubtypeToString


static const char *partitionTypeToString(esp_partition_type_t type) {
	switch(type) {
	case ESP_PARTITION_TYPE_APP:
		return "APP";
	case ESP_PARTITION_TYPE_DATA:
		return "DATA";
	default:
		return "Unknown";
	}
} // partitionTypeToString


/**
 * Build an return an array of partitions.
 * [0] - Type of partition either "app" or "data"
 *
 * The return data is an array of object of the form
 * {
 *    "type":    <String>
 *    "subtype": <String>
 *    "label":   <String>
 *    "address": <number>
 *    "size":    <number>
 * }
 */
static duk_ret_t js_partitions_list(duk_context *ctx) {
	esp_partition_type_t partitionType;
	esp_partition_iterator_t it;

	if (!duk_is_string(ctx, -1)) {
		ESP_LOGD(tag, "js_partitions_list not passed a string.");
		return 0;
	}
	const char *typeString = duk_get_string(ctx, -1);
	if (strcmp(typeString, "app") == 0) {
		partitionType = ESP_PARTITION_TYPE_APP;
	} else if (strcmp(typeString, "data") == 0) {
		partitionType = ESP_PARTITION_TYPE_DATA;
	} else {
		ESP_LOGD(tag, "js_partitions_list not passed either \"app\" or \"data\".");
		return 0;
	}

	duk_idx_t resultArrayIdx = duk_push_array(ctx);
	// [0] - Type of partition either "app" or "data"
	// [1] - New result array
	int i=0;
	it = esp_partition_find(partitionType, ESP_PARTITION_SUBTYPE_ANY, NULL);
	while(it != NULL) {
		const esp_partition_t *partition = esp_partition_get(it);
		duk_idx_t objIdx = duk_push_object(ctx);
		// [0] - Type of partition either "app" or "data"
		// [1] - New result array
		// [2] - New object

		duk_push_string(ctx, partitionTypeToString(partition->type));
		// [0] - Type of partition either "app" or "data"
		// [1] - New result array
		// [2] - New object
		// [3] - partition type

		duk_put_prop_string(ctx, objIdx, "type");
		// [0] - Type of partition either "app" or "data"
		// [1] - New result array
		// [2] - New object

		duk_push_string(ctx, partitionSubtypeToString(partition->subtype));
		// [0] - Type of partition either "app" or "data"
		// [1] - New result array
		// [2] - New object
		// [3] - partition subtype

		duk_put_prop_string(ctx, objIdx, "subtype");
		// [0] - Type of partition either "app" or "data"
		// [1] - New result array
		// [2] - New object

		duk_push_string(ctx, partition->label);
		// [0] - Type of partition either "app" or "data"
		// [1] - New result array
		// [2] - New object
		// [3] - partition label

		duk_put_prop_string(ctx, objIdx, "label");
		// [0] - Type of partition either "app" or "data"
		// [1] - New result array
		// [2] - New object

		duk_push_int(ctx, partition->address);
		// [0] - Type of partition either "app" or "data"
		// [1] - New result array
		// [2] - New object
		// [3] - partition address

		duk_put_prop_string(ctx, objIdx, "address");
		// [0] - Type of partition either "app" or "data"
		// [1] - New result array
		// [2] - New object

		duk_push_int(ctx, partition->size);
		// [0] - Type of partition either "app" or "data"
		// [1] - New result array
		// [2] - New object
		// [3] - partition size

		duk_put_prop_string(ctx, objIdx, "size");
		// [0] - Type of partition either "app" or "data"
		// [1] - New result array
		// [2] - New object

		duk_put_prop_index(ctx, resultArrayIdx, i);
		i++;
		// [0] - Type of partition either "app" or "data"
		// [1] - New result array

		LOGD("Partition: %s %s %s 0x%x[0x%x]",
				partitionTypeToString(partition->type),
				partitionSubtypeToString(partition->subtype),
				partition->label,
				partition->address,
				partition->size);

		it = esp_partition_next(it);
	}
	return 1;
} // js_partitions_list


/**
 * Create the PARTITIONS module in the new object
 * [0] - Partitions object.
 */
duk_ret_t ModulePartitions(duk_context *ctx) {

	ADD_FUNCTION("list", js_partitions_list, 1);

	return 0;
} // module_partitions;
