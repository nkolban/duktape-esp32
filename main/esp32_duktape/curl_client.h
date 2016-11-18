/*
 * curl_client.h
 *
 *  Created on: Nov 13, 2016
 *      Author: kolban
 */

#ifndef COMPONENTS_JERRYSCRIPT_ESP32_INCLUDE_JERRYSCRIPT_ESP32_CURL_CLIENT_H_
#define COMPONENTS_JERRYSCRIPT_ESP32_INCLUDE_JERRYSCRIPT_ESP32_CURL_CLIENT_H_
#include "module_http.h"

char *curl_client_getContent(const char *url);
void jerryscript_esp32_register_load();
void curl_client_http_request(struct http_request_options *pOptions);

#endif /* COMPONENTS_JERRYSCRIPT_ESP32_INCLUDE_JERRYSCRIPT_ESP32_CURL_CLIENT_H_ */
