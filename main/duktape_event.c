#if defined(ESP_PLATFORM)
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <esp_log.h>
#include "sdkconfig.h"
#endif // ESP_PLATFORM

#include <assert.h>
#include <stdbool.h>
#include <string.h>

#include "c_timeutils.h"
#include "duktape_event.h"
#include "duktape_utils.h"
#include "logging.h"

LOG_TAG("duktape_event");

// The maximum number of concurrent events we can have on the
// queue for processing.
#define MAX_EVENT_QUEUE_SIZE (20)

#if defined(ESP_PLATFORM)
static QueueHandle_t esp32_duktape_event_queue; // The event queue (provided by FreeRTOS).
#endif /* ESP_PLATFORM */

static void postEvent(esp32_duktape_event_t *pEvent, bool isISR);


/**
 * Mark the event as completed by releasing any state associated with it.
 */
void esp32_duktape_freeEvent(duk_context *ctx, esp32_duktape_event_t *pEvent) {
	if(pEvent->type == ESP32_DUKTAPE_EVENT_COMMAND_LINE) {
		free(pEvent->commandLine.commandLine);
		return;
	}


	if (pEvent->type == ESP32_DUKTAPE_EVENT_CALLBACK_REQUESTED) {
		if (pEvent->callbackRequested.callbackType == ESP32_DUKTAPE_CALLBACK_TYPE_FUNCTION) {
			esp32_duktape_stash_delete(ctx, pEvent->callbackRequested.stashKey);
		}
		return;
	}

	LOGD("We have been asked to free an event of type %d but don't know how",
		pEvent->type);
} // esp32_duktape_freeEvent


/**
 * Initialize the event handling.
 */
void esp32_duktape_initEvents() {
	// Initialize the FreeRTOS queue.
#ifdef ESP_PLATFORM
	esp32_duktape_event_queue = xQueueCreate(MAX_EVENT_QUEUE_SIZE, sizeof(esp32_duktape_event_t));
#endif
} // esp32_duktape_initEvents


/**
 * Check to see if there is an event to process.
 *
 * We return 0 to indicate that no event was caught.
 */
int esp32_duktape_waitForEvent(esp32_duktape_event_t *pEvent) {
	// Ask FreeRTOS to see if we have a new event.
#if defined(ESP_PLATFORM)
	BaseType_t rc = xQueueReceive(esp32_duktape_event_queue, pEvent, 0);
#else /* ESP_PLATFORM */
	int rc = 0;
#endif /* ESP_PLATFORM */
	return (int)rc;
} // esp32_duktape_waitForEvent


/**
 * Convert an event type to a string representation.
 */
char *event_eventTypeToString(int eventType) {
	switch(eventType) {
		case ESP32_DUKTAPE_EVENT_COMMAND_LINE:
			return "ESP32_DUKTAPE_EVENT_COMMAND_LINE";
		case ESP32_DUKTAPE_EVENT_CALLBACK_REQUESTED:
			return "ESP32_DUKTAPE_EVENT_CALLBACK_REQUESTED";
		default:
			return "Unknown event type";
	}
} // event_eventTypeToString


/**
 * Build and post a new CallbackRequestedEvent.  This kind of event is used to indicate
 * to the runtime that a C function outside the context of any JavaScript framework
 * wishes a function to be invoked.  The function will be found identified by the
 * stashKey parameter.
 * * callbackType - The type of callback.  Choices are:
 *   - ESP32_DUKTAPE_CALLBACK_TYPE_FUNCTION - General function callback.  This is a one time callback
 *      and the data associated with it will be deleted after being called.
 * * stashKey - A key to an array stash which holds the JS function to be called plus any
 *              parameters to that function.
 * * dataProvider - a function that will be called to add parameters to the eventual JS callback.
 *                  This can be NULL if no such function is desired.
 * * contextData - ??
 */
void event_newCallbackRequestedEvent(
	uint32_t callbackType,
	uint32_t stashKey,
	esp32_duktape_callback_dataprovider dataProvider,
	void *contextData) {

	LOGD(">> event_newCallbackRequestedEvent stashKey=%d", stashKey);
	esp32_duktape_event_t event;
	event.type = ESP32_DUKTAPE_EVENT_CALLBACK_REQUESTED;
	if (callbackType != ESP32_DUKTAPE_CALLBACK_TYPE_FUNCTION &&
			callbackType != ESP32_DUKTAPE_CALLBACK_TYPE_ISR_FUNCTION) {
		LOGE("event_newCallbackRequestedEvent: Unknown callbackType: %d", callbackType);
		return;
	}
	event.callbackRequested.callbackType = callbackType;
	event.callbackRequested.stashKey     = stashKey;
	event.callbackRequested.dataProvider = dataProvider;
	event.callbackRequested.context      = contextData;
	if (callbackType == ESP32_DUKTAPE_CALLBACK_TYPE_ISR_FUNCTION) {
		postEvent(&event, true);
	} else {
		postEvent(&event, false);
	}
	LOGD("<< event_newCallbackRequestedEvent");
} // event_newCallbackRequestedEvent


/**
 * Post a new command line event.  The commandData is expected to have been
 * malloced() and should NOT be freed by the caller.  It will be freed
 * when the event has completed being processed.
 */
void event_newCommandLineEvent(
		char *commandData, // The data received from the command input.
		size_t commandLength, // The length of the data received.
		int fromKeyboard // True if it came from the keyboard
	) {
	esp32_duktape_event_t event;

	if (commandLength == 0 || commandData == NULL) {
		LOGE("event_newCommandLineEvent: problem ... length of command was %d which should be > 0 or commandData was NULL=%s ", (int)commandLength, commandData==NULL?"yes":"no");
		return;
	}
	assert(commandLength > 0);
	assert(commandData != NULL);

	event.commandLine.type = ESP32_DUKTAPE_EVENT_COMMAND_LINE;
	event.commandLine.commandLine = malloc(commandLength);
	assert(event.commandLine.commandLine != NULL);

	memcpy(event.commandLine.commandLine, commandData, commandLength);
	event.commandLine.commandLineLength = commandLength;
	event.commandLine.fromKeyboard = fromKeyboard;

	postEvent(&event, false); // Post the event.
} //newCommandLineEvent


/**
 * Post the event onto the queue for handling when idle.
 */
static void postEvent(esp32_duktape_event_t *pEvent, bool isISR) {
#if defined(ESP_PLATFORM)
	if (isISR) {
		xQueueSendToBackFromISR(esp32_duktape_event_queue, pEvent, NULL);
	} else {
		xQueueSendToBack(esp32_duktape_event_queue, pEvent, portMAX_DELAY);
	}
#else /* ESP_PLATFORM */
	assert(0);
#endif  /* ESP_PLATFORM */
} // postEvent
