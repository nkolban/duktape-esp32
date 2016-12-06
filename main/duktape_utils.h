/*
 * duktape_utils.h
 *
 *  Created on: Nov 18, 2016
 *      Author: kolban
 */

#ifndef MAIN_DUKTAPE_UTILS_H_
#define MAIN_DUKTAPE_UTILS_H_
#include <duktape.h>
void esp32_duktape_console(const char *message);
void esp32_duktape_addGlobalFunction(
		duk_context *ctx,
		char *functionName,
		duk_c_function func,
		duk_int_t numArgs);
void esp32_duktape_set_reset(int value);
int esp32_duktape_is_reset();
void esp32_duktape_dump_value_stack(duk_context *ctx);

void esp32_duktape_stash_init(duk_context *ctx);
void esp32_duktape_unstash_object(duk_context *ctx, uint32_t key);
void esp32_duktape_unstash_array(duk_context *ctx, uint32_t key);
void esp32_duktape_stash_delete(duk_context *ctx, uint32_t key);
uint32_t esp32_duktape_stash_object(duk_context *ctx, int count);
uint32_t esp32_duktape_stash_array(duk_context *ctx, int count);
#endif /* MAIN_DUKTAPE_UTILS_H_ */
