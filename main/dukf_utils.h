/*
 * dukf_utils.h
 *
 *  Created on: Dec 17, 2016
 *      Author: kolban
 */

#ifndef MAIN_DUKF_UTILS_H_
#define MAIN_DUKF_UTILS_H_
#include <duktape.h>
#include <stdint.h>
#include <unistd.h>

const char *dukf_loadFile(const char *path, size_t *fileSize);
void dukf_addRunAtStart(const char *fileName);
void dukf_runAtStart(duk_context *ctx);
void dukf_runFile(duk_context *ctx, const char *fileName);
void dukf_log_heap(char *tag);
#endif /* MAIN_DUKF_UTILS_H_ */
