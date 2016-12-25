/*
 * duktape_fixup.h
 *
 *  Created on: Dec 25, 2016
 *      Author: kolban
 */

#ifndef MAIN_INCLUDE_DUKTAPE_FIXUP_H_
#define MAIN_INCLUDE_DUKTAPE_FIXUP_H_
#include <stdio.h>
#include <sys/time.h>
/**
 * This function must return the number of milliseconds since the 1970
 * epoch.  We use gettimeofday() provided by the environment.  The name
 * of the function must be specified in duk_config.h ... for example:
 *
 * #define DUK_USE_DATE_GET_NOW(ctx) esp32_duktape_get_now()
 * #define DUK_USE_DATE_GET_LOCAL_TZOFFSET(d)  0
 */
duk_double_t esp32_duktape_get_now();

#endif /* MAIN_INCLUDE_DUKTAPE_FIXUP_H_ */
