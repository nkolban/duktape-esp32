/*
 * duktape_event.h
 *
 *  Created on: Nov 19, 2016
 *      Author: kolban
 */

#if !defined(MAIN_ESP32_DUKTAPE_DUKTAPE_EVENT_H_)
#define MAIN_ESP32_DUKTAPE_DUKTAPE_EVENT_H_
#include <stdlib.h>
#include <stdint.h>
#include <duktape.h>
enum {
	ESP32_DUKTAPE_EVENT_COMMAND_LINE,
	ESP32_DUKTAPE_EVENT_HTTPSERVER_REQUEST,
	ESP32_DUKTAPE_EVENT_TIMER_ADDED,
	ESP32_DUKTAPE_EVENT_TIMER_FIRED,
	ESP32_DUKTAPE_EVENT_TIMER_CLEARED,
	ESP32_DUKTAPE_EVENT_CALLBACK_REQUESTED
};

enum {
	ESP32_DUKTAPE_CALLBACK_TYPE_FUNCTION=1
};

/*
 * The signature of a data provide function.  When called it will place 0 or more
 * values on the callstack and return how many were added.
 */
typedef int (*esp32_duktape_callback_dataprovider)(duk_context *ctx, void *context);

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
		uint32_t stashKey;
		esp32_duktape_callback_dataprovider dataProvider; // A C function to be called that will push values onto the call stack
		void *context; // Context data to add to be passed to the dataProvider
	} callbackRequested;
} esp32_duktape_event_t;

void  esp32_duktape_freeEvent(duk_context *ctx, esp32_duktape_event_t *pEvent);
void  esp32_duktape_initEvents();
int   esp32_duktape_waitForEvent();
char *event_eventTypeToString(int eventType);
void  event_newCallbackRequestedEvent(
	uint32_t callbackType,
	uint32_t stashKey,
	esp32_duktape_callback_dataprovider dataProvider,
	void *contextData);
void  event_newCommandLineEvent(char *commandData, size_t commandLength, int fromKeyboard);
void  event_newHTTPServerRequestEvent(char *uri, char *method);
void  event_newTimerAddedEvent(unsigned long);
void  event_newTimerClearedEvent(unsigned long id);
void  event_newTimerFiredEvent(unsigned long id);

#endif /* MAIN_ESP32_DUKTAPE_DUKTAPE_EVENT_H_ */
