/*
 * c_timeutils.h
 *
 *  Created on: Nov 26, 2016
 *      Author: kolban
 */

#ifndef MAIN_ESP32_DUKTAPE_C_TIMEUTILS_H_
#define MAIN_ESP32_DUKTAPE_C_TIMEUTILS_H_
#include <sys/time.h>
void timeval_addMsecs(struct timeval *a, uint32_t msecs);
uint32_t timeval_toMsecs(struct timeval *a);
struct timeval timeval_sub(struct timeval *a, struct timeval *b);
struct timeval timeval_add(struct timeval *a, struct timeval *b);
uint32_t timeval_durationFromNow(struct timeval *a);

#endif /* MAIN_ESP32_DUKTAPE_C_TIMEUTILS_H_ */
