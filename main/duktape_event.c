#if defined(ESP_PLATFORM)
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <esp_log.h>
#include "sdkconfig.h"
#endif // ESP_PLATFORM

#include <assert.h>
#include <string.h>

#include "c_timeutils.h"
#include "duktape_event.h"
#include "duktape_utils.h"
#include "logging.h"
#include "module_timers.h"

LOG_TAG("duktape_event");

// The maximum number of concurrent events we can have on the
// queue for processing.
#define MAX_EVENT_QUEUE_SIZE (20)

#if defined(ESP_PLATFORM)
static QueueHandle_t esp32_duktape_event_queue; // The event queue (provided by FreeRTOS).
#endif /* ESP_PLATFORM */

/**
 * Post the event onto the queue for handling when idle.
 */
static void postEvent(esp32_duktape_event_t *pEvent) {
#if defined(ESP_PLATFORM)
	xQueueSendToBack(esp32_duktape_event_queue, pEvent, portMAX_DELAY);
#else /* ESP_PLATFORM */
	assert(0);
#endif  /* ESP_PLATFORM */
} // postEvent


/**
 * Convert an event type to a string representation.
 */
char *event_eventTypeToString(int eventType) {
	switch(eventType) {
		case ESP32_DUKTAPE_EVENT_COMMAND_LINE:
			return "ESP32_DUKTAPE_EVENT_COMMAND_LINE";
		case ESP32_DUKTAPE_EVENT_HTTPSERVER_REQUEST:
			return "ESP32_DUKTAPE_EVENT_HTTPSERVER_REQUEST";
		case ESP32_DUKTAPE_EVENT_TIMER_ADDED:
			return "ESP32_DUKTAPE_EVENT_TIMER_ADDED";
		case ESP32_DUKTAPE_EVENT_TIMER_FIRED:
			return "ESP32_DUKTAPE_EVENT_TIMER_FIRED";
		case ESP32_DUKTAPE_EVENT_TIMER_CLEARED:
			return "ESP32_DUKTAPE_EVENT_TIMER_CLEARED";
		case ESP32_DUKTAPE_EVENT_CALLBACK_REQUESTED:
			return "ESP32_DUKTAPE_EVENT_CALLBACK_REQUESTED";
		default:
			return "Unknown event type";
	}
} // event_eventTypeToString


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
 * Mark the event as completed by releasing any state associated with it.
 */
void esp32_duktape_freeEvent(duk_context *ctx, esp32_duktape_event_t *pEvent) {
	if(pEvent->type == ESP32_DUKTAPE_EVENT_COMMAND_LINE) {
		free(pEvent->commandLine.commandLine);
		return;
	}

	if (pEvent->type == ESP32_DUKTAPE_EVENT_HTTPSERVER_REQUEST) {
		free(pEvent->httpServerRequest.method);
		free(pEvent->httpServerRequest.uri);
		return;
	}
	if (pEvent->type == ESP32_DUKTAPE_EVENT_TIMER_ADDED) {
		// Nothing clean up for this event type.
		return;
	}

	if (pEvent->type == ESP32_DUKTAPE_EVENT_TIMER_CLEARED) {
		// Nothing clean up for this event type.
		return;
	}

	if (pEvent->type == ESP32_DUKTAPE_EVENT_TIMER_FIRED) {
		// Nothing clean up for this event type.
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

	postEvent(&event); // Post the event.
} //newCommandLineEvent


/**
 * Create an event that indicates a new HTTP Server request has been
 * received from outside of Duktape.
 */
/*
void event_newHTTPServerRequestEvent(
		char *uri,
		char *method
	) {
	esp32_duktape_event_t event;

	event.httpServerRequest.type = ESP32_DUKTAPE_EVENT_HTTPSERVER_REQUEST;
	event.httpServerRequest.method = method;
	event.httpServerRequest.uri = uri;

	postEvent(&event);
} // newHTTPServerRequestEvent
*/

/**
 * Create an event that indicates we have received a request to insert a new
 * timer into our timer chain.
 */
/*
void event_newTimerAddedEvent(unsigned long id) {
	esp32_duktape_event_t event;
	event.type = ESP32_DUKTAPE_EVENT_TIMER_ADDED;
	postEvent(&event);
} // event_newTimerAddedEvent
*/

/**
 * Indicate that a timer was cleared.
 */
/*
void event_newTimerClearedEvent(unsigned long id) {
	esp32_duktape_event_t event;
	event.type = ESP32_DUKTAPE_EVENT_TIMER_CLEARED;
	postEvent(&event);
} // event_newTimerClearedEvent
*/

/**
 * indicate that a timer has fired.
 */
/*
void event_newTimerFiredEvent(unsigned long id) {
	esp32_duktape_event_t event;
	event.type = ESP32_DUKTAPE_EVENT_TIMER_FIRED;
	event.timerFired.id = id;
	postEvent(&event);
} // event_newTimerFiredEvent
*/

/**
 * Build and post a new CallbackRequestedEvent.  This kind of event is used to indicate
 * to the runtime that a C function outside the context of any JavaScript framework
 * wishes a function to be invoked.  The function will be found identified by the
 * stashKey parameter.
 */
void event_newCallbackRequestedEvent(
	uint32_t callbackType,
	uint32_t stashKey,
	esp32_duktape_callback_dataprovider dataProvider,
	void *contextData) {

	LOGD(">> event_newCallbackRequestedEvent stashKey=%d", stashKey);
	esp32_duktape_event_t event;
	event.type = ESP32_DUKTAPE_EVENT_CALLBACK_REQUESTED;
	if (callbackType != ESP32_DUKTAPE_CALLBACK_TYPE_FUNCTION) {
		LOGE("event_newCallbackRequestedEvent: Unknown callbackType: %d", callbackType);
		return;
	}
	event.callbackRequested.callbackType = callbackType;
	event.callbackRequested.stashKey     = stashKey;
	event.callbackRequested.dataProvider = dataProvider;
	event.callbackRequested.context      = contextData;
	postEvent(&event);
	LOGD("<< event_newCallbackRequestedEvent");
} // event_newCallbackRequestedEvent
