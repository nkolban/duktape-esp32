/**
 * Duktape environmental and task control functions.
 */
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdlib.h>
#include <esp_log.h>
#include <duktape.h>
#include "esp32_duktape/duktape_event.h"
#include "modules.h"
#include "telnet.h"
#include "duktape_utils.h"
#include "duktape_task.h"
#include "sdkconfig.h"

static char tag[] = "duktape_task";

// The Duktape context.
duk_context *esp32_duk_context;


/**
 * Read a line of text from the queue of line records and blocking if none
 * are available.  The line is copied into the passed in buffer and the
 * length of the line read is returned.  How the line is added to the queue
 * is of no importance to the caller of this function.
 */
size_t readLine(char *buffer, int maxLen) {
	ESP_LOGD(tag, ">> readline");
	line_record_t line_record;
	size_t retSize;
	BaseType_t rc = xQueueReceive(line_records_queue, &line_record, portMAX_DELAY);
	if (rc == pdTRUE) {
		if (line_record.length > maxLen) {
			line_record.length = maxLen;
		}
		memcpy(buffer, line_record.data, line_record.length);
		free(line_record.data);
		retSize = line_record.length;
	} else {
		retSize = 0;
	}
	ESP_LOGD(tag, "<< readline");
	return retSize;
} // End of readLine


/**
 * Initialize the duktape environment.
 */
void duktape_init_environment() {
	// If we currently have an existing environment release it because we are
	// about to create a new one.
	if (esp32_duk_context != NULL) {
		duk_destroy_heap(esp32_duk_context);
	}

	esp32_duk_context = duk_create_heap_default();	// Create the Duktape context.
	registerModules(esp32_duk_context); // Register the built-in modules

	// Print a console logo.
	esp32_duktape_console(
	"\n ______  _____ _____ ____ ___\n"
	"|  ____|/ ____|  __ \\___ \\__ \\\n"
	"| |__  | (___ | |__) |__) | ) |\n"
	"|  __|  \\___ \\|  ___/|__ < / /\n"
	"| |____ ____) | |    ___) / /_\n"
	"|______|_____/|_|  _|____/____|\n"
	"|  __ \\      | |  | |\n"
	"| |  | |_   _| | _| |_ __ _ _ __   ___ \n"
	"| |  | | | | | |/ / __/ _` | '_ \\ / _ \\\n"
	"| |__| | |_| |   <| || (_| | |_) |  __/\n"
	"|_____/ \\__,_|_|\\_\\\\__\\__,_| .__/ \\___|\n"
	"                           | |\n"
	"                           |_|\n"
	" http://duktape.org\n"
	" ESP32 port/framework: Neil Kolban\n"
	);
	esp32_duktape_set_reset(0); // Flag the environment as having been reset.
	esp32_duktape_console("esp32_duktape> "); // Log a prompt.
} // duktape_init_environment


void processEvent(esp32_duktape_event_t *pEvent) {
	switch(pEvent->type) {
	case ESP32_DUKTAPE_EVENT_COMMAND_LINE: {
		duk_int_t evalResponse = duk_peval_lstring(esp32_duk_context, pEvent->commandLine.commandLine, pEvent->commandLine.commandLineLength);
		if (evalResponse != 0) {
			esp32_duktape_console(duk_safe_to_string(esp32_duk_context, -1));
			esp32_duktape_console("\n");
		}
		duk_pop(esp32_duk_context); // Discard the result from the stack.
		esp32_duktape_console("esp32_duktape> "); // Put out a prompt.
		break;
	}
	case ESP32_DUKTAPE_EVENT_HTTPSERVER_REQUEST:
		ESP32_LOGD(tag, "Process a webserver (inbound) request ...");
		break;
	default:
		break;
	}
}

/**
 * Start the duktape processing.
 *
 * Here we loop waiting for a line of input to be received and, when
 * it arrives, we process it through an eval().  The input comes from
 * a call to readLine() which insulates us from the actual source of
 * the line.
 */
void duktape_task(void *ignore) {
	ESP_LOGD(tag, ">> duktape_task");
	char cmd[256];
	size_t len;
	int evalResponse; // Response value from a JS eval

	duktape_init_environment();

	while(1) {

		/*
		// Wait for an event;
		// Process an event;
		 */
		len = readLine(cmd, sizeof(cmd));
		//ESP_LOGD(tag, "Passing %.*s to duktape", len, cmd);

		// This is the key execution area.  Here we pass the line to the JS runtime
		// for execution.
		evalResponse = duk_peval_lstring(esp32_duk_context, cmd, len);
		if (evalResponse != 0) {
			esp32_duktape_console(duk_safe_to_string(esp32_duk_context, -1));
			esp32_duktape_console("\n");
		}
		duk_pop(esp32_duk_context);

		esp32_duktape_console("esp32_duktape> ");
		//("Unhandled JS exception occured: ");
		//print_native_error(ret_val);
		//esp32_duktape_console("\n");
		//ESP_LOGD(tag, "Result is %s", duk_get_string(esp32_duk_context, -1));


		// If we have been requested to reset the environment
		// then do that now.
		if (esp32_duktape_is_reset()) {
			duktape_init_environment();
		}

	} // End while loop.
	ESP_LOGD(tag, "<< duktape_task");
	vTaskDelete(NULL);
} // End of duktape_task
