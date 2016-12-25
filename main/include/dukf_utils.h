/*
 * dukf_utils.h
 *
 *  Created on: Dec 17, 2016
 *      Author: kolban
 */

#if !defined(MAIN_DUKF_UTILS_H_)
#define MAIN_DUKF_UTILS_H_
#include <duktape.h>
#include <stdint.h>
#include <unistd.h>

void        dukf_addRunAtStart(const char *fileName);
uint32_t    dukf_get_free_heap_size();
const char *dukf_loadFile(const char *path, size_t *fileSize);
void        dukf_log_heap(const char *tag);
void        dukf_runAtStart(duk_context *ctx);
void        dukf_runFile(duk_context *ctx, const char *fileName);

#endif /* MAIN_DUKF_UTILS_H_ */
