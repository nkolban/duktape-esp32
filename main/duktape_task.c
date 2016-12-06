/**
 * Duktape environmental and task control functions.
 */
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdlib.h>
#include <esp_log.h>
#include <esp_err.h>
#include <esp_wifi.h>
#include <duktape.h>
#include <spiffs.h>
#include <esp_spiffs.h>
#include "esp32_duktape/duktape_event.h"
#include "esp32_duktape/module_timers.h"
#include "modules.h"
#include "telnet.h"
#include "duktape_utils.h"
#include "duktape_task.h"
#include "duktape_spiffs.h"
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
	esp32_duktape_stash_init(esp32_duk_context); // Initialize the stash environment.
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
 * Convert an authentication mode to a string.
 */
static char *authModeToString(wifi_auth_mode_t mode) {
	switch(mode) {
	case WIFI_AUTH_OPEN:
		return "open";
	case WIFI_AUTH_WEP:
		return "wep";
	case WIFI_AUTH_WPA_PSK:
		return "wpa";
	case WIFI_AUTH_WPA2_PSK:
		return "wpa2";
	case WIFI_AUTH_WPA_WPA2_PSK:
		return "wpa_wpa2";
	default:
		return "unknown";
	}
} // authModeToString


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
 *
 * * ESP32_DUKTAPE_EVENT_COMMAND_LINE - A new user entered text line for processing.
 * * ESP32_DUKTAPE_EVENT_HTTPSERVER_REQUEST - A new browser request has arrived for
 *   us while we are being a web server.
 * * ESP32_DUKTAPE_EVENT_TIMER_ADDED - A timer has been added.
 * * ESP32_DUKTAPE_EVENT_TIMER_CLEARED - A timer has been cleared.
 * * ESP32_DUKTAPE_EVENT_TIMER_FIRED - A timer has been fired.
 * * ESP32_DUKTAPE_EVENT_WIFI_SCAN_COMPLETED - A WiFi scan has completed.
 */
void processEvent(esp32_duktape_event_t *pEvent) {
	ESP_LOGD(tag, ">> processEvent");
	switch(pEvent->type) {
		// Handle a new command line submitted to us.
		case ESP32_DUKTAPE_EVENT_COMMAND_LINE: {
			ESP_LOGD(tag, "We are about to eval: %.*s", pEvent->commandLine.commandLineLength, pEvent->commandLine.commandLine);
			duk_peval_lstring(esp32_duk_context,
				pEvent->commandLine.commandLine, pEvent->commandLine.commandLineLength);
			// [0] - result

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

		// Handle a new externally initiated browser request arriving at us.
		case ESP32_DUKTAPE_EVENT_HTTPSERVER_REQUEST: {
			ESP_LOGD(tag, "Process a webserver (inbound) request event ... uri: %s, method: %s",
					pEvent->httpServerRequest.uri,
					pEvent->httpServerRequest.method);
			// Find the global function called _httpServerRequestReceivedCallback and
			// invoke it.  We pass in any necessary parameters.

			// Push the global object onto the stack
			// Retrieve the _httpServerRequestReceivedCallback function
			duk_bool_t rcB = duk_get_global_string(esp32_duk_context, "_httpServerRequestReceivedCallback");
			// [0] - _httpServerRequestReceivedCallback
			if (rcB) {
				duk_push_string(esp32_duk_context, pEvent->httpServerRequest.uri);
				// [0] - _httpServerRequestReceivedCallback
				// [1] - uri

				duk_push_string(esp32_duk_context, pEvent->httpServerRequest.method);
				// [0] - _httpServerRequestReceivedCallback
				// [1] - uri
				// [2] - method

				duk_call(esp32_duk_context, 2);
				// [0] - Return val

				duk_pop(esp32_duk_context);
				// Empty stack
			} else {
				duk_pop(esp32_duk_context);
				// Empty stack
			}

			break;
		}

		case ESP32_DUKTAPE_EVENT_TIMER_ADDED: {
			ESP_LOGD(tag, "Process a timer added event");
			break;
		}

		case ESP32_DUKTAPE_EVENT_TIMER_FIRED: {
			ESP_LOGD(tag, "Process a timer fired event: %lu", pEvent->timerFired.id);
			timers_runTimer(esp32_duk_context, pEvent->timerFired.id);
			break;
		}

		case ESP32_DUKTAPE_EVENT_TIMER_CLEARED: {
			ESP_LOGD(tag, "Process a timer cleared event");
			break;
		}

		case ESP32_DUKTAPE_EVENT_CALLBACK_REQUESTED: {
			ESP_LOGD(tag, "Process a callback requested event: callbackType=%d, contextData=0x%x, callData=0x%x",
				pEvent->callbackRequested.callbackType,
				(uint32_t)pEvent->callbackRequested.context,
				(uint32_t)pEvent->callbackRequested.data
			);
			// Now that we have a callback request, we pass control back into JS by calling a JS
			// function.  The JS function we call is a global called "eventCallback".
			duk_push_global_object(esp32_duk_context);
			// [0] - Global object

			if (duk_get_prop_string(esp32_duk_context, -1, "eventCallback")) {
				// [0] - Global object <object>
				// [1] - eventCallback <function>

				duk_push_int(esp32_duk_context, pEvent->callbackRequested.callbackType);
				// [0] - Global object <object>
				// [1] - eventCallback <function>
				// [2] - callbackType <number>

				duk_push_heapptr(esp32_duk_context, pEvent->callbackRequested.context);
				// [0] - Global object <object>
				// [1] - eventCallback <function>
				// [2] - callbackType <number>
				// [3] - context <object>

				duk_push_string(esp32_duk_context, (char *)pEvent->callbackRequested.data);
				// [0] - Global object <object>
				// [1] - eventCallback <function>
				// [2] - callbackType <number>
				// [3] - context <object>
				// [4] - data <String>

				duk_json_decode(esp32_duk_context, -1);
				// [0] - Global object <object>
				// [1] - eventCallback <function>
				// [2] - callbackType <number>
				// [3] - context <object>
				// [4] - data <Object>


				duk_pcall(esp32_duk_context, 3 /* Number of parms */);
				// [0] - Global object <object>
				// [1] - result
			} else {
				ESP_LOGD(tag, "Unable to find global function called eventCallback");

				duk_pop(esp32_duk_context);
				// [0] - Global object
				// [1] - undefined
			}
			duk_pop_2(esp32_duk_context);
			// Empty Stack

			break;
		}

		// Handle the event that a Wifi scan has completed.  Our logic hear is to build an
		// object that contains the results and then call each of the callback functions
		// that want to hear about WiFi scans passing them the object.  The list of callbacks
		// is contained in an array called "scan_callbacks_array" that is found on the
		// heapStash.  It may not exist and it may be empty (if something has gone wrong or
		// we have executed two scans in quick succession).
		// A published scan record will contain:
		// {
		//    ssid: <network id>,
		//    mac: <mac address>
		//    rssi: <signal strength>
		//    auth: <authentication mode>
		// }
		//
		case ESP32_DUKTAPE_EVENT_WIFI_SCAN_COMPLETED: {
			int i;

			ESP_LOGD(tag, "Process a scan result event.");

			// Get the number of access points in our last scan.
			uint16_t numAp;
			ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&numAp));

			// Allocate storage for our scan results and retrieve them.
			wifi_ap_record_t *apRecords = calloc(sizeof(wifi_ap_record_t), numAp);
			ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&numAp, apRecords));
			ESP_LOGD(tag, "Number of Access Points: %d", numAp);

			// Build an array for the results ...
			duk_idx_t resultIdx = duk_push_array(esp32_duk_context);
			// [0] - Array - results

			// Add each of the scan results into the array.
			for (i=0; i<numAp; i++) {
				ESP_LOGD(tag, "%d: ssid=%s", i, apRecords[i].ssid);
				duk_idx_t scanResultIdx = duk_push_object(esp32_duk_context); // Create a new scan result object
				// [0] - Array - results
				// [1] - New object

				duk_push_string(esp32_duk_context, (char *)apRecords[i].ssid);
				// [0] - Array - results
				// [1] - New object
				// [2] - String (ssid)

				duk_put_prop_string(esp32_duk_context, scanResultIdx, "ssid");
				// [0] - Array - results
				// [1] - New object

				duk_push_sprintf(esp32_duk_context, MACSTR, MAC2STR(apRecords[i].bssid));
				// [0] - Array - results
				// [1] - New object
				// [2] - String (mac)

				duk_put_prop_string(esp32_duk_context, scanResultIdx, "mac");
				// [0] - Array - results
				// [1] - New object

				duk_push_int(esp32_duk_context, apRecords[i].rssi);
				// [0] - Array - results
				// [1] - New object
				// [2] - Number (rssi)

				duk_put_prop_string(esp32_duk_context, scanResultIdx, "rssi");
				// [0] - Array - results
				// [1] - New object

				duk_push_string(esp32_duk_context, authModeToString(apRecords[i].authmode));
				// [0] - Array - results
				// [1] - New object
				// [2] - String (auth)

				duk_put_prop_string(esp32_duk_context, scanResultIdx, "auth");
				// [0] - Array - results
				// [1] - New object

				duk_put_prop_index(esp32_duk_context, resultIdx, i); // Add the new record into the results array
				// [0] - Array - results
			}

			// We now have a WiFi scan completed and we are in the Duktape thread, so let us
			// invoke the callbacks for those scans (assuming they exist).
			duk_push_heap_stash(esp32_duk_context);
			// [0] - Array - results
			// [1] - Heap stash

			if (!duk_get_prop_string(esp32_duk_context, -1, "scan_callbacks_array")) {
				// [0] - Array - results
				// [1] - Heap stash
				// [2] - scan_callbacks array (undefined)
				ESP_LOGE(tag, "Want to process Scan callbacks, but no callbacks found!");
				duk_pop_3(esp32_duk_context);
				break;
			}

			// [0] - Array - results
			// [1] - Heap stash
			// [2] - scan_callbacks array

			duk_size_t length = duk_get_length(esp32_duk_context, -1);
			ESP_LOGD(tag, "Number of callbacks to invoke: %d", length);

			for (i=0; i<length; i++) {
				int rc = duk_get_prop_index(esp32_duk_context, -1, i);
				// [0] - Array - results
				// [1] - Heap stash
				// [2] - scan_callbacks array
				// [3] - Callback function
				if (rc == 0) {
					ESP_LOGD(tag, "Unable to find callback: %d in scan_callbacks_array", i);
				}

				duk_dup(esp32_duk_context, resultIdx);
				// [0] - Array - results
				// [1] - Heap stash
				// [2] - scan_callbacks array
				// [3] - Callback function
				// [4] - Array - results

				duk_pcall(esp32_duk_context, 1); // Invoke the callback with 1 parameter
				// [0] - Array - results
				// [1] - Heap stash
				// [2] - scan_callbacks array
				// [3] - scan callback result

				duk_pop(esp32_duk_context); // Discard the result from the call
				// [0] - Array - results
				// [1] - Heap stash
				// [2] - scan_callbacks array
			}

			// Now that we have called all the callbacks, time to empty the callback array.
			duk_push_int(esp32_duk_context, 0);
			// [0] - Array - results
			// [1] - Heap stash
			// [2] - scan_callbacks array
			// [3] - number (0)

			duk_put_prop_string(esp32_duk_context, -2, "length"); // Empty the callbacks array
			// [0] - Array - results
			// [1] - Heap stash
			// [2] - scan_callbacks array

			duk_pop_3(esp32_duk_context); // Clean the stack
			// <Empty Stack>
			break;
		}

		default:
			break;
	} // End of switch
	ESP_LOGD(tag, "<< processEvent");
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
	duk_idx_t lastTop;
	int rc;

	ESP_LOGD(tag, ">> duktape_task");

	// Mount the SPIFFS file system.
	esp32_duktape_spiffs_mount();

	duktape_init_environment();
	// From here on, we have a Duktape context ...


	lastTop = duk_get_top(esp32_duk_context); // Get the last top value of the stack from which we will use to check for leaks.

	//
	// Master JavaScript loop.
	//
	while(1) {
		rc = esp32_duktape_waitForEvent(&esp32_duktape_event);
		if (rc != 0) {
			processEvent(&esp32_duktape_event);
			esp32_duktape_freeEvent(&esp32_duktape_event);
		}

		// If we have been requested to reset the environment
		// then do that now.
		if (esp32_duktape_is_reset()) {
			duktape_init_environment();
		}

		// Check for value stack leakage
		if (duk_get_top(esp32_duk_context) != lastTop) {
			ESP_LOGE(tag, "We have detected that the stack has leaked!");
			esp32_duktape_dump_value_stack(esp32_duk_context);
			lastTop = duk_get_top(esp32_duk_context);
		} // End of check for value stack leakage.

	} // End while loop.

	// We should never reach here ...
	ESP_LOGD(tag, "<< duktape_task");
	vTaskDelete(NULL);
} // End of duktape_task
