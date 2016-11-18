#include <esp_log.h>
#include <duktape.h>
#include <stdlib.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "sdkconfig.h"

static char tag[] = "test_duktape";
void test_duktape(void * data) {
	ESP_LOGD(tag, ">> test_duktape");
  duk_context *ctx = duk_create_heap_default();
  duk_eval_string(ctx, "print('Hello world!');");
  duk_destroy_heap(ctx);
	ESP_LOGD(tag, "<< test_duktape");
	vTaskDelete(NULL);
}
