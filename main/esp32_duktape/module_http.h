/*
 * module_http.h
 *
 *  Created on: Nov 15, 2016
 *      Author: kolban
 */

#ifndef COMPONENTS_JERRYSCRIPT_ESP32_INCLUDE_JERRYSCRIPT_ESP32_MODULE_HTTP_H_
#define COMPONENTS_JERRYSCRIPT_ESP32_INCLUDE_JERRYSCRIPT_ESP32_MODULE_HTTP_H_
struct http_request_options {
	char *protocol;
	char *host;
	uint16_t port;
	char *method;
	char *path;
};

#endif /* COMPONENTS_JERRYSCRIPT_ESP32_INCLUDE_JERRYSCRIPT_ESP32_MODULE_HTTP_H_ */
