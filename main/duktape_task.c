/**
 * Duktape environmental and task control functions.

 */
#if defined(ESP_PLATFORM)

#include <esp_err.h>
#include <esp_log.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <spiffs.h>

#include "duktape_spiffs.h"
#include "esp32_specific.h"
#include "sdkconfig.h"

#endif // ESP_PLATFORM

#include <assert.h>
#include <duktape.h>
#include <stdlib.h>

#include "duk_module_duktape.h"
#include "dukf_utils.h"
#include "duktape_task.h"
#include "duktape_utils.h"
#include "duktape_event.h"
#include "logging.h"
#include "modules.h"
//#include "telnet.h"

LOG_TAG("duktape_task");

// The Duktape context.
duk_context *esp32_duk_context;

/*
 * Log a logo to the console.
 */
static void showLogo() {
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
	" Build: " __DATE__ " " __TIME__ "\n"
	"\n"
	);
} // showLogo


/**
 * Initialize the duktape environment.
 */
void duktape_init_environment() {


	// If we currently have an existing environment release it because we are
	// about to create a new one.
	if (esp32_duk_context != NULL) {
		duk_destroy_heap(esp32_duk_context);
	}

	LOGD("About to create heap");
	esp32_duk_context = duk_create_heap_default();	// Create the Duktape context.
	dukf_log_heap("Heap after duk create heap");

	//duk_eval_string_noresult(esp32_duk_context, "Duktape = Object.create(Duktape);");
	duk_eval_string_noresult(esp32_duk_context, "new Function('return this')().Duktape = Object.create(Duktape);");

	duk_module_duktape_init(esp32_duk_context); // Initialize the duktape module functions.
	dukf_log_heap("Heap after duk_module_duktape_init");

	esp32_duktape_stash_init(esp32_duk_context); // Initialize the stash environment.

	registerModules(esp32_duk_context); // Register the built-in modules
	dukf_log_heap("Heap after duk register modules");

	esp32_duktape_set_reset(0); // Flag the environment as having been reset.

	dukf_runFile(esp32_duk_context, "/init.js");	// Load and run the script called "init.js"
	/*
	LOGD("Running \"init.js\"");
	size_t fileSize;
	const char *data = dukf_loadFileFromESPFS("init.js", &fileSize);
	assert(data != NULL);

	duk_push_lstring(esp32_duk_context, data, fileSize);
	int rc = duk_peval(esp32_duk_context);
	if (rc != 0) {
		esp32_duktape_log_error(esp32_duk_context);
	}
	duk_pop(esp32_duk_context);
	*/

	dukf_log_heap("Heap after init.js");

	esp32_duktape_console("esp32_duktape> "); // Log a prompt.
} // duktape_init_environment


/**
 * Process an event within the context of JavaScript.  JavaScript is a serialized
 * language which means that we can't do multi threading or other parallel activities.
 * This can appear to be a problem as JavaScript also lends itself to async processing.
 * For example, we might have a network or file operation that takes time and
 * when complete, invokes a callback to say "we are done".  However, in a serial
 * world, we must not interrupt what we are currently doing.  Hence we introduce the
 * notion of events and queues of events.  We postulate the existence of an event
 * queue which holds events (that happened externally) and are ready for processing.
 * Since this is also a queue, we have a notion of first in, first out.  When our
 * JavaScript program is "idle", we can take the next item from the event queue (or
 * block waiting for an item to arrive should it be empty) and then process that event.
 * This then repeats forever.
 *
 * This routine is the function which processes a single event.  The event types we
 * have defined so far are:
 *
 * * ESP32_DUKTAPE_EVENT_COMMAND_LINE - A new user entered text line for processing.
 * * ESP32_DUKTAPE_EVENT_CALLBACK_REQUESTED - A request to run a callback has been received.
 */

void processEvent(esp32_duktape_event_t *pEvent) {
	duk_int_t callRc;
	LOGV(">> processEvent: eventType=%s", event_eventTypeToString(pEvent->type));
	switch(pEvent->type) {
		// Handle a new command line submitted to us.
		case ESP32_DUKTAPE_EVENT_COMMAND_LINE: {
			LOGD("We are about to eval: %.*s", pEvent->commandLine.commandLineLength, pEvent->commandLine.commandLine);
			callRc = duk_peval_lstring(esp32_duk_context,	pEvent->commandLine.commandLine, pEvent->commandLine.commandLineLength);
			// [0] - result

// If an error was detected, perform error logging.
			if (callRc != 0) {
				esp32_duktape_log_error(esp32_duk_context);
			}

			// If we executed from a keyboard, send keyboard user response.
			if (pEvent->commandLine.fromKeyboard) {
				esp32_duktape_console(duk_safe_to_string(esp32_duk_context, -1));
				esp32_duktape_console("\n");
				esp32_duktape_console("esp32_duktape> "); // Put out a prompt.
			}

			duk_pop(esp32_duk_context); // Discard the result from the stack.
			// <Empty Stack>
			break;
		}

		case ESP32_DUKTAPE_EVENT_CALLBACK_REQUESTED: {

			// The event contains 4 properties:
			// * callbackType - int
			// * stashKey     - int - A key to an array stash that contains a function and parameters.
			// * context      - void * - a Duktape heapptr
			// * dataProvider - A function to be called that will add parameters to the stack.  If NULL, then
			//                  no data provider function is present.
			LOGD("Process a callback requested event: callbackType=%d, stashKey=%d",
				pEvent->callbackRequested.callbackType,
				pEvent->callbackRequested.stashKey
			);
			if (pEvent->callbackRequested.callbackType == ESP32_DUKTAPE_CALLBACK_TYPE_FUNCTION ||
					pEvent->callbackRequested.callbackType == ESP32_DUKTAPE_CALLBACK_TYPE_ISR_FUNCTION) {

				int topStart = duk_get_top(esp32_duk_context);

				// In this case, the stashKey points to a stashed array which starts with a callback function and parameters.
				esp32_duktape_unstash_array(esp32_duk_context, pEvent->callbackRequested.stashKey);
				// [0] - function
				// [1] - param 1
				// [.] - param
				// [n] - param last

				int numberParams = duk_get_top(esp32_duk_context) - topStart -1;

				LOGD("ESP32_DUKTAPE_EVENT_CALLBACK_REQUESTED: #params: %d", numberParams);

				if (!duk_is_function(esp32_duk_context, topStart)) {
					LOGE("ESP32_DUKTAPE_EVENT_CALLBACK_REQUESTED: Not a function!");
					duk_pop_n(esp32_duk_context, duk_get_top(esp32_duk_context) - topStart);
					return;
				}

				if (pEvent->callbackRequested.dataProvider != NULL) {
					int numberAdditionalStackItems = pEvent->callbackRequested.dataProvider(esp32_duk_context, pEvent->callbackRequested.context);
					numberParams += numberAdditionalStackItems;
				}

				duk_pcall(esp32_duk_context, numberParams);
				// [0] - Ret val

				duk_pop(esp32_duk_context);
				// <empty stack>
			}

			break;
		} // End of ESP32_DUKTAPE_EVENT_CALLBACK_REQUESTED

		default:
			LOGD("Unknown event type seen: %d", pEvent->type);
			break;
	} // End of switch
	LOGV("<< processEvent");
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
	esp32_duktape_event_t esp32_duktape_event;
	int rc;

	LOGD(">> duktape_task");
	dukf_log_heap("duktape_task");

	dukf_init_nvs_values(); // Initialize any defaults for NVS data

	// Define the scripts which are to run at boot time
	dukf_addRunAtStart("start.js");
	//dukf_addRunAtStart("bootwifi.js");
	//dukf_addRunAtStart("webserver.js");
	//dukf_addRunAtStart("test_ide_ws.js");

	// Mount the SPIFFS file system.
#if defined(ESP_PLATFORM)
	esp32_duktape_spiffs_mount();
#endif /* ESP_PLATFORM */


	duktape_init_environment();
	// From here on, we have a Duktape context ...

	duk_idx_t lastStackTop = duk_get_top(esp32_duk_context); // Get the last top value of the stack from which we will use to check for leaks.

	// Run the one time startup scripts
	dukf_runAtStart(esp32_duk_context);

	//
	// Master JavaScript loop.
	//
#if defined(ESP_PLATFORM)
	LOGD("Free heap at start of JavaScript main loop: %d", esp_get_free_heap_size());
#endif /* ESP_PLATFORM */

	showLogo();

	LOGD("Starting main loop!");
	while(1) {
		// call the loop routine.
		duk_push_global_object(esp32_duk_context);
		duk_get_prop_string(esp32_duk_context, -1, "_loop");
		assert(duk_is_function(esp32_duk_context, -1));

		rc = duk_pcall(esp32_duk_context, 0);
		if (rc != 0) {
#if defined(ESP_PLATFORM)
			LOGD("Error running loop!  free heap=%d", esp_get_free_heap_size());
#endif /* ESP_PLATFORM */
			esp32_duktape_log_error(esp32_duk_context);
		}
		duk_pop_2(esp32_duk_context);
		// We have ended the loop routine.


		rc = esp32_duktape_waitForEvent(&esp32_duktape_event);
		if (rc != 0) {
			processEvent(&esp32_duktape_event);
			esp32_duktape_freeEvent(esp32_duk_context, &esp32_duktape_event);
		}

		// If we have been requested to reset the environment
		// then do that now.
		if (esp32_duktape_is_reset()) {
			duktape_init_environment();
		}

		// Check for value stack leakage.
		if (duk_get_top(esp32_duk_context) != lastStackTop) {
			LOGE("We have detected that the stack has leaked!");
			esp32_duktape_dump_value_stack(esp32_duk_context);
			lastStackTop = duk_get_top(esp32_duk_context);
		} // End of check for value stack leakage.

#if defined(ESP_PLATFORM)
		//taskYIELD();
		vTaskDelay(1);
/*
		uint32_t heapSize = esp_get_free_heap_size();
		if (heapSize < 10000) {
			LOGV("heap: %d",heapSize);
		}
*/
#endif /* ESP_PLATFORM */

	} // End while loop.

	// We should never reach here ...
	LOGD("<< duktape_task");

#if defined(ESP_PLATFORM)
	// We are not allowed to end a task routine without first deleting the task.
	vTaskDelete(NULL);
#endif /* ESP_PLATFORM */
} // End of duktape_task
