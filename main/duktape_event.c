#include "esp32_duktape/duktape_event.h"
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <esp_log.h>
#include <assert.h>
#include "esp32_duktape/module_timers.h"
#include "c_timeutils.h"
#include "sdkconfig.h"

static char tag[] = "duktape_event";

// The maximum number of concurrent events we can have on the
// queue for processing.
#define MAX_EVENT_QUEUE_SIZE (20)

static QueueHandle_t esp32_duktape_event_queue; // The event queue (provided by FreeRTOS).

/**
 * Post the event onto the queue for handling when idle.
 */
static void postEvent(esp32_duktape_event_t *pEvent) {
	xQueueSendToBack(esp32_duktape_event_queue, pEvent, portMAX_DELAY);
} // postEvent


/**
 * Initialize the event handling.
 */
void esp32_duktape_initEvents() {
	// Initialize the FreeRTOS queue.
	esp32_duktape_event_queue = xQueueCreate(MAX_EVENT_QUEUE_SIZE, sizeof(esp32_duktape_event_t));
} // esp32_duktape_initEvents


/**
 * Wait for an event blocking if there isn't one on the event queue ready
 * for processing.  We also handle here the concept of timers.  We block here
 * until an event occurs or else a timeout expires and the timeout is the time
 * of the next timer to wake up.
 *
 * We return 0 to indicate that no event was caught.
 */
int esp32_duktape_waitForEvent(esp32_duktape_event_t *pEvent) {
	BaseType_t waitDelay = portMAX_DELAY;

	// Get the next timer to expire (if there is one) and, define the blocking period
	// to be the time when we must wake up to process it.
	timer_record_t *pTimerRecord = timers_getNextTimer();
	if (pTimerRecord != NULL) {
		waitDelay = timeval_durationFromNow(&pTimerRecord->wakeTime) / portTICK_PERIOD_MS;
	}

	// Ask FreeRTOS to block us until either the timer expires or a new event
	// is added to the event queue.
	BaseType_t rc = xQueueReceive(esp32_duktape_event_queue, pEvent, waitDelay);

	// If the rc is false, then we did NOT get a new event which means that we
	// timed out.
	if (rc == pdFALSE) {
		assert(pTimerRecord != NULL); // If it is NULL, why did we wake up?
		event_newTimerFiredEvent(pTimerRecord->id); // Post an event that a timer fired.
	}
	return (int)rc;
} // esp32_duktape_waitForEvent


/**
 * Mark the event as completed by releasing any state associated with it.
 */
void esp32_duktape_freeEvent(esp32_duktape_event_t *pEvent) {
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

	if (pEvent->type == ESP32_DUKTAPE_EVENT_WIFI_SCAN_COMPLETED) {
		// Nothing to clean up for this event type.
		return;
	}

	ESP_LOGD(tag, "We have been asked to free an event of type %d but don't know how",
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

	event.commandLine.type = ESP32_DUKTAPE_EVENT_COMMAND_LINE;
	event.commandLine.commandLine = malloc(commandLength);
	memcpy(event.commandLine.commandLine, commandData, commandLength);
	event.commandLine.commandLineLength = commandLength;
	event.commandLine.fromKeyboard = fromKeyboard;

	postEvent(&event); // Post the event.
} //newCommandLineEvent


/**
 * Create an event that indicates a new HTTP Server request has been
 * received from outside of Duktape.
 */
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


/**
 * Create an event that indicates we have received a request to insert a new
 * timer into our timer chain.
 */
void event_newTimerAddedEvent(unsigned long id) {
	esp32_duktape_event_t event;
	event.type = ESP32_DUKTAPE_EVENT_TIMER_ADDED;
	postEvent(&event);
} // event_newTimerAddedEvent


/**
 * Indicate that a timer was cleared.
 */
void event_newTimerClearedEvent(unsigned long id) {
	esp32_duktape_event_t event;
	event.type = ESP32_DUKTAPE_EVENT_TIMER_CLEARED;
	postEvent(&event);
} // event_newTimerClearedEvent


/**
 * indicate that a timer has fired.
 */
void event_newTimerFiredEvent(unsigned long id) {
	esp32_duktape_event_t event;
	event.type = ESP32_DUKTAPE_EVENT_TIMER_FIRED;
	event.timerFired.id = id;
	postEvent(&event);
} // newTimerFiredEvent

void event_newWifiScanCompletedEvent() {
	esp32_duktape_event_t event;
	event.type = ESP32_DUKTAPE_EVENT_WIFI_SCAN_COMPLETED;
	postEvent(&event);
} // event_newWifiScanCompletedEvent
