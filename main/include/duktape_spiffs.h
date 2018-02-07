/*
 * duktape_spiffs.h
 *
 *  Created on: Dec 3, 2016
 *      Author: kolban
 */

#if !defined(MAIN_DUKTAPE_SPIFFS_H_)
#define MAIN_DUKTAPE_SPIFFS_H_
#include <esp_spiffs.h>
#include <duktape.h>

void esp32_duktape_dump_spiffs();
void esp32_duktape_dump_spiffs_array(duk_context *ctx);
void esp32_duktape_spiffs_mount();

#endif /* MAIN_DUKTAPE_SPIFFS_H_ */
