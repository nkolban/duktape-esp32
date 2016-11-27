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
 * for processing.
 *
 * We return 0 to indicate that no event was caught.
 */
int esp32_duktape_waitForEvent(esp32_duktape_event_t *pEvent) {
	BaseType_t waitDelay = portMAX_DELAY;
	timer_record_t *pTimerRecord = timers_getNextTimer();
	if (pTimerRecord != NULL) {
		waitDelay = timeval_durationFromNow(&pTimerRecord->wakeTime) / portTICK_PERIOD_MS;
	}
	BaseType_t rc = xQueueReceive(esp32_duktape_event_queue, pEvent, waitDelay);
	if (rc == pdFALSE) {
		// OOOoh!! Did a timer expire!?
		ESP_LOGD(tag, "We think a timer expired!");
		assert(pTimerRecord != NULL); // If it is NULL, why did we wake up?
		event_newTimerFiredEvent(pTimerRecord->id);
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
		// Nothing to do for this event type.
		return;
	}
} // esp32_duktape_freeEvent


/**
 * Post a new command line event.  The commandData is expected to have been
 * malloced() and should NOT be freed by the caller.  It will be freed
 * when the event is processed.
 */
void event_newCommandLineEvent(
		char *commandData,
		size_t commandLength,
		int fromKeyboard
	) {
	esp32_duktape_event_t event;

	event.commandLine.type = ESP32_DUKTAPE_EVENT_COMMAND_LINE;
	event.commandLine.commandLine = malloc(commandLength);
	memcpy(event.commandLine.commandLine, commandData, commandLength);
	event.commandLine.commandLineLength = commandLength;
	event.commandLine.fromKeyboard = fromKeyboard;
	postEvent(&event);
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


void event_newTimerClearedEvent(unsigned long id) {
	esp32_duktape_event_t event;
	event.type = ESP32_DUKTAPE_EVENT_TIMER_CLEARED;
	postEvent(&event);
} // event_newTimerClearedEvent


void event_newTimerFiredEvent(unsigned long id) {
	esp32_duktape_event_t event;
	event.type = ESP32_DUKTAPE_EVENT_TIMER_FIRED;
	event.timerFired.id = id;
	postEvent(&event);
} // newTimerFiredEvent
