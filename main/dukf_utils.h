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

char *dukf_loadFile(char *path, size_t *fileSize);
void dukf_addRunAtStart(char *fileName);
void dukf_runAtStart(duk_context *ctx);
void dukf_runFile(duk_context *ctx, char *fileName);
#endif /* MAIN_DUKF_UTILS_H_ */
