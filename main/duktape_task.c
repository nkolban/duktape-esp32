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
	" ESP32 port/framework: Neil Kolban\n\n"
	);
	esp32_duktape_set_reset(0); // Flag the environment as having been reset.
	esp32_duktape_console("esp32_duktape> "); // Log a prompt.
} // duktape_init_environment


/**
 * Process an event within the context of JavaScript.  JavaScript is a serialized
 * language which means that we can't do multi threading or other parallel activities.
 * This can appear to be a problem as JavaScript also lends itself to async processing.
 * For example, we might have a network or file operation that takes time and
 * when complete, invokes a callback to say "we are done".  However, in a serial
 * world, we must not interrupt what we are currently doing.  Hence we introduce the
 * notion of events and queues of events.  We postualte the existence of an event
 * queue which holds events (that happened externally) and are ready for processing.
 * Since this is also a queue, we have a notion of first in, first out.  When our
 * JavaScript program is "idle", we can take the next item from the event queue (or
 * block waiting for an item to arrive should it be empty) and then process that event.
 * This then repeats forever.
 *
 * This routine is the function which processes a single event.  The event types we
 * have defined so far are:
 * * ESP32_DUKTAPE_EVENT_COMMAND_LINE - A new user entered text line for processing.
 * * ESP32_DUKTAPE_EVENT_HTTPSERVER_REQUEST - A new browser request has arrived for
 *   us while we are being a web server.
 */
void processEvent(esp32_duktape_event_t *pEvent) {
	switch(pEvent->type) {
		// Handle a new command line submitted to us.
		case ESP32_DUKTAPE_EVENT_COMMAND_LINE: {
			ESP_LOGD(tag, "We are about to eval: %.*s", pEvent->commandLine.commandLineLength, pEvent->commandLine.commandLine);
			duk_peval_lstring(esp32_duk_context,
				pEvent->commandLine.commandLine, pEvent->commandLine.commandLineLength);

			// If we executed from a keyboard, send keyboard user response.
			if (pEvent->commandLine.fromKeyboard) {
				esp32_duktape_console(duk_safe_to_string(esp32_duk_context, -1));
				esp32_duktape_console("\n");
				esp32_duktape_console("esp32_duktape> "); // Put out a prompt.
			}

			duk_pop(esp32_duk_context); // Discard the result from the stack.
			break;
		}

		// Handle a new externally initiated browser request arriving at us.
		case ESP32_DUKTAPE_EVENT_HTTPSERVER_REQUEST:
			ESP_LOGD(tag, "Process a webserver (inbound) request event ... uri: %s, method: %s",
					pEvent->httpServerRequest.uri,
					pEvent->httpServerRequest.method);
			// Find the global function called _httpServerRequestReceivedCallback and
			// invoke it.  We pass in any necessary parameters.

			// Push the global object onto the stack
			// Retrieve the _httpServerRequestReceivedCallback function
			duk_bool_t rcB = duk_get_global_string(esp32_duk_context, "_httpServerRequestReceivedCallback");
			if (rcB) {
				duk_push_string(esp32_duk_context, pEvent->httpServerRequest.uri);
				duk_push_string(esp32_duk_context, pEvent->httpServerRequest.method);
				duk_call(esp32_duk_context, 2);
			}

			// Push that onto the stack
			// Push the parameters onto the stack
			// Call the function.
			break;

		default:
			break;
	}
} // processEvent


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
	esp32_duktape_event_t esp32_duktape_event;

	duktape_init_environment();

	//
	// Master JavaScript loop.
	//
	while(1) {
		int rc = esp32_duktape_waitForEvent(&esp32_duktape_event);
		if (rc != 0) {
			processEvent(&esp32_duktape_event);
			esp32_duktape_freeEvent(&esp32_duktape_event);
		}

		// If we have been requested to reset the environment
		// then do that now.
		if (esp32_duktape_is_reset()) {
			duktape_init_environment();
		}

	} // End while loop.

	// We should never reach here ...
	ESP_LOGD(tag, "<< duktape_task");
	vTaskDelete(NULL);
} // End of duktape_task
