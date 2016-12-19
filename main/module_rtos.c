#include <duktape.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "logging.h"

LOG_TAG("module_rtos");

static duk_ret_t js_rtos_systemState(duk_context *ctx) {
#if configUSE_TRACE_FACILITY == 1
	LOGD(">> js_rtos_systemState");
	UBaseType_t count = uxTaskGetNumberOfTasks();
	LOGD("Task count: %d", count);
	TaskStatus_t *statusData = calloc(sizeof(TaskStatus_t), count);
	UBaseType_t rc = uxTaskGetSystemState(
		statusData,
	  count,
	  NULL);
	int i;
	TaskStatus_t *pStatus = statusData;
	for (i=0; i<rc; i++, pStatus++) {
		LOGD("Task: id=%d, name=%s, stackHigh=%d, runTimeCounter=%d, currentPriority=%d, basePriority=%d",
				pStatus->xTaskNumber,
				pStatus->pcTaskName,
				pStatus->usStackHighWaterMark,
				pStatus->ulRunTimeCounter,
				pStatus->uxCurrentPriority,
				pStatus->uxBasePriority);
	}
	free(statusData);
	LOGD("<< js_rtos_systemState");
	return 0;
#else
	LOGD(">> js_rtos_systemState not enabled ... configUSE_TRACE_FACILITY <> 1");
	return 0;
#endif
}

/**
 * Create the RTOS module in Global.
 */
void ModuleRTOS(duk_context *ctx) {
	duk_push_global_object(ctx);
	// [0] - Global object


	duk_idx_t idx = duk_push_object(ctx); // Create new RTOS object
	// [0] - Global object
	// [1] - New object


	duk_push_c_function(ctx, js_rtos_systemState, 0);
	// [0] - Global object
	// [1] - New object
	// [2] - c-func - js_rtos_systemState


	duk_put_prop_string(ctx, idx, "systemState"); // Add systemState to new RTOS
	// [0] - Global object
	// [1] - New object


	duk_put_prop_string(ctx, 0, "RTOS"); // Add RTOS to global
	// [0] - Global object

	duk_pop(ctx);
} // ModuleRTOS
