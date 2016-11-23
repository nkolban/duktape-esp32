#include "esp32_duktape/duktape_event.h"
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

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
 */
int esp32_duktape_waitForEvent(esp32_duktape_event_t *pEvent) {
	BaseType_t rc = xQueueReceive(esp32_duktape_event_queue, pEvent, portMAX_DELAY);
	return (int)rc;
} // esp32_duktape_waitForEvent


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
} // esp32_duktape_freeEvent

/**
 * Post a new command line event.  The commandData is expected to have been
 * malloced() and should NOT be freed by the caller.  It will be freed
 * when the event is processed.
 */
void newCommandLineEvent(
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


void newHTTPServerRequestEvent(
		char *uri,
		char *method
	) {
	esp32_duktape_event_t event;

	event.httpServerRequest.type = ESP32_DUKTAPE_EVENT_HTTPSERVER_REQUEST;
	event.httpServerRequest.method = method;
	event.httpServerRequest.uri = uri;
	postEvent(&event);
} // newHTTPServerRequestEvent
