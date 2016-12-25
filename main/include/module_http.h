/*
 * module_http.h
 *
 *  Created on: Nov 15, 2016
 *      Author: kolban
 */

#if !defined(COMPONENTS_JERRYSCRIPT_ESP32_INCLUDE_ESP32_MODULE_HTTP_H_)
#define COMPONENTS_JERRYSCRIPT_ESP32_INCLUDE_ESP32_MODULE_HTTP_H_
#include "duktape.h"

void ModuleHTTP(duk_context *ctx);
typedef struct http_request_options {
	char *protocol;
	char *host;
	uint16_t port;
	char *method;
	char *path;
} http_request_options_t;

#endif /* COMPONENTS_JERRYSCRIPT_ESP32_INCLUDE_ESP32_MODULE_HTTP_H_ */
