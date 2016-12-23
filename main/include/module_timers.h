/*
 * module_gpio.h
 *
 *  Created on: Nov 18, 2016
 *      Author: kolban
 */

#if !defined(MAIN_ESP32_DUKTAPE_MODULE_TIMERS_H_)
#define MAIN_ESP32_DUKTAPE_MODULE_TIMERS_H_
#include <sys/time.h>
#include <stdint.h>
#include <duktape.h>

typedef struct {
	struct timeval duration; // The duation to wait for (relative).
	struct timeval wakeTime; // The real time to wake up at (absolute).
	uint8_t isInterval; // True if this is an interval timer, false if it is a timeout timer;.
	unsigned long id; // Id of the timer.
} timer_record_t;

/**
 * Get the next timer to fire.  NULL if there is no next timer.
 */
timer_record_t *timers_getNextTimer();


/**
 * Run the function associated with the specified timer.
 */
void timers_runTimer(duk_context *ctx, unsigned long id);

/**
 * Delete all the timers.
 */
void timers_deleteAll(duk_context *ctx);

void ModuleTIMERS(duk_context *ctx);

#endif /* MAIN_ESP32_DUKTAPE_MODULE_TIMERS_H_ */
