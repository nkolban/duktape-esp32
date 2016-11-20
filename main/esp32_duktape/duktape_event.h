/*
 * duktape_event.h
 *
 *  Created on: Nov 19, 2016
 *      Author: kolban
 */

#ifndef MAIN_ESP32_DUKTAPE_DUKTAPE_EVENT_H_
#define MAIN_ESP32_DUKTAPE_DUKTAPE_EVENT_H_

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
	} httpRequest;
} esp32_duktape_event_t;

#endif /* MAIN_ESP32_DUKTAPE_DUKTAPE_EVENT_H_ */
