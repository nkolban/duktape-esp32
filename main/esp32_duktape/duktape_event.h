/*
 * duktape_event.h
 *
 *  Created on: Nov 19, 2016
 *      Author: kolban
 */

#ifndef MAIN_ESP32_DUKTAPE_DUKTAPE_EVENT_H_
#define MAIN_ESP32_DUKTAPE_DUKTAPE_EVENT_H_
#include <stdlib.h>
enum {
	ESP32_DUKTAPE_EVENT_COMMAND_LINE,
	ESP32_DUKTAPE_EVENT_HTTPSERVER_REQUEST
};
typedef union {
	int type;
	struct {
		int type;
		char *commandLine;
		int commandLineLength;
	} commandLine;
	struct {
		int type;
		// HTTP parts
		char *uri;
		char *method;
	} httpServerRequest;
} esp32_duktape_event_t;

esp32_duktape_event_t *newCommandLineEvent(esp32_duktape_event_t *pEvent, char *commandData, size_t commandLength);
esp32_duktape_event_t *newHTTPServerRequestEvent(
		esp32_duktape_event_t *pEvent,
		char *uri,
		char *method
	);
void esp32_duktape_initEvents();
int esp32_duktape_waitForEvent();
void esp32_duktape_freeEvent(esp32_duktape_event_t *pEvent);
#endif /* MAIN_ESP32_DUKTAPE_DUKTAPE_EVENT_H_ */
