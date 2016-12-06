/*
 * duktape_event.h
 *
 *  Created on: Nov 19, 2016
 *      Author: kolban
 */

#ifndef MAIN_ESP32_DUKTAPE_DUKTAPE_EVENT_H_
#define MAIN_ESP32_DUKTAPE_DUKTAPE_EVENT_H_
#include <stdlib.h>
#include <stdint.h>
enum {
	ESP32_DUKTAPE_EVENT_COMMAND_LINE,
	ESP32_DUKTAPE_EVENT_HTTPSERVER_REQUEST,
	ESP32_DUKTAPE_EVENT_TIMER_ADDED,
	ESP32_DUKTAPE_EVENT_TIMER_FIRED,
	ESP32_DUKTAPE_EVENT_TIMER_CLEARED,
	ESP32_DUKTAPE_EVENT_WIFI_SCAN_COMPLETED,
	ESP32_DUKTAPE_EVENT_CALLBACK_REQUESTED,
};

// !!! IMPORTANT !!!
// All event types MUST start with an int type
typedef union {
	int type;
	// ESP32_DUKTAPE_EVENT_COMMAND_LINE
	struct {
		int type;
		// Command line parts
		char *commandLine;
		int commandLineLength;
		int fromKeyboard;
	} commandLine;

	// ESP32_DUKTAPE_EVENT_HTTPSERVER_REQUEST
	struct {
		int type;
		// HTTP parts
		char *uri;
		char *method;
	} httpServerRequest;

	// ESP32_DUKTAPE_EVENT_TIMER_FIRED
	struct {
		int type;
		// Timer fired parts
		unsigned long id;
	} timerFired;

	// ESP32_DUKTAPE_EVENT_CALLBACK_REQUESTED
	struct {
		int type;
		uint32_t callbackType;
		void *context;
		char *data;
	} callbackRequested;
} esp32_duktape_event_t;

void event_newTimerAddedEvent(unsigned long);
void event_newTimerClearedEvent(unsigned long id);
void event_newCommandLineEvent(char *commandData, size_t commandLength, int fromKeyboard);
void event_newHTTPServerRequestEvent(char *uri, char *method);
void event_newTimerFiredEvent(unsigned long id);
void event_newWifiScanCompletedEvent();
void event_newCallbackRequestedEvent(uint32_t callbackType, void *contextData, char *data);

void esp32_duktape_initEvents();
int esp32_duktape_waitForEvent();
void esp32_duktape_freeEvent(esp32_duktape_event_t *pEvent);
#endif /* MAIN_ESP32_DUKTAPE_DUKTAPE_EVENT_H_ */
