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
#else /* configUSE_TRACE_FACILITY */
	LOGD(">> js_rtos_systemState not enabled ... configUSE_TRACE_FACILITY <> 1");
	return 0;
#endif /* configUSE_TRACE_FACILITY */
} // js_rtos_systemState


/**
 * Create the RTOS module in Global.
 * [0] - RTOS Object to be populated.
 */
duk_ret_t ModuleRTOS(duk_context *ctx) {

	duk_push_c_function(ctx, js_rtos_systemState, 0);
	// [0] - RTOS object
	// [1] - c-func - js_rtos_systemState


	duk_put_prop_string(ctx, -2, "systemState"); // Add systemState to new RTOS
	// [0] - RTOS object

	duk_pop(ctx);

	return 0;
} // ModuleRTOS
