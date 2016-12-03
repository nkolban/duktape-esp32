/*
 * esp32_mongoose.h
 *
 *  Created on: Nov 19, 2016
 *      Author: kolban
 */

#ifndef MAIN_ESP32_MONGOOSE_H_
#define MAIN_ESP32_MONGOOSE_H_
#include <duktape.h>

duk_ret_t js_startMongoose(duk_context *ctxParam);
duk_ret_t js_serverResponseMongoose(duk_context *ctxParam);
void startMongooseServer();
void websocket_console_sendData(const char *message);
void esp32_mongoose_sendHTTPRequest(const char *url);

#endif /* MAIN_ESP32_MONGOOSE_H_ */
