/*
 * esp32_mongoose.h
 *
 *  Created on: Nov 19, 2016
 *      Author: kolban
 */

#ifndef MAIN_ESP32_MONGOOSE_H_
#define MAIN_ESP32_MONGOOSE_H_
#include <duktape.h>

duk_ret_t startMongoose(duk_context *ctxParam);
duk_ret_t serverResponseMongoose(duk_context *ctxParam);

#endif /* MAIN_ESP32_MONGOOSE_H_ */
