/**
 * Implement the timers functions.
 *
 * This file provides the implementation for the following global JS functions:
 *
 * * setInterval
 * * setTimeout
 * * clearInterval
 * * clearTimeout
 *
 * The functions that are to be executed in the future have to be not
 * eligible for garbage collection and also be able to be retrieved
 * in the future.  To achieve this we use heap stash object.  Within this
 * object we create a new object called "timers" which is a map keyed by the
 * timer id.  The value of this property will be the function reference.
 *
 * We expose the following functional capabilities:
 *
 * timers_deleteAll    - Delete all the timer state.
 * timers_getNextTimer - Get the next timer that is due to fire.
 * timers_runTimer     - Run the callback function associated with a timer id.
 *
 */
#include <stdbool.h>
#include <esp_log.h>
#include <esp_system.h>
#include <duktape.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>

#include "c_list.h"
#include "c_timeutils.h"
#include "duktape_utils.h"
#include "duktape_event.h"
#include "module_timers.h"
#include "sdkconfig.h"

static timer_record_t *getById(unsigned long id);

static char tag[] = "module_timers";

static uint32_t g_nextId = 0; // The next timer id

static list_t *timerList = NULL; // The list of timer records.

/**
 * Determine whether or not timeVal a is sooner than timeVal b.  If it is
 * then 1 is returned otherwise 0 is returned.
 */
static int sooner(timer_record_t *a, timer_record_t *b) {
	if (a->wakeTime.tv_sec < b->wakeTime.tv_sec) {
		return 1;
	}
	if (a->wakeTime.tv_sec == b->wakeTime.tv_sec &&
			a->wakeTime.tv_usec < b->wakeTime.tv_usec) {
		return 1;
	}
	return 0;
} // sooner


/**
 * Delete the timer.  The timer is identified with the id parameter.
 * The timer's data exists in two distinct places.  First there is the
 * entry in the timers list and second is the entry in <heapStash>.timers.
 * Both have to be removed.
 */
static void deleteTimer(duk_context *ctx, unsigned long id) {

	// Here we delete the property from the <heapStash>.timers object with
	// property key given by the timer id.

	// [0] - heap stash
	duk_push_heap_stash(ctx);

	// [0] - heap stash
	// [1] - timers
	duk_get_prop_string(ctx, -1, "timers");

	// [0] - heap stash
	// [1] - timers (-2)
	// [2] - key (timer id) (-1)
	duk_push_int(ctx, id);

	// [0] - heap stash
	// [1] - timers
	duk_del_prop(ctx, -2);

	// <empty>
	duk_pop_2(ctx);

	list_t *pListRecord = list_first(timerList);
	while(pListRecord != NULL) {
		if (((timer_record_t *)pListRecord->value)->id == id) {
			list_delete(timerList, pListRecord, 1);
			break;
		}
		pListRecord = list_next(pListRecord);
	} // End while
} // deleteTimer


/**
 * Get the next timer that is to fire.  If there is no next timer, then
 * NULL is returned.
 */
timer_record_t *timers_getNextTimer() {

	list_t *pListRecord = list_first(timerList);
	list_t *pSoonest = pListRecord;
	while(pListRecord != NULL) {
		if (sooner((timer_record_t *)pListRecord->value, (timer_record_t *)pSoonest->value)) {
			pSoonest = pListRecord;
		}
		pListRecord = list_next(pListRecord);
	}
	// We may not actually HAVE a next timer record.
	if (pSoonest == NULL) {
		return NULL;
	}
	// Since we DO have a next timer record at this point, return it.
	return (timer_record_t *)pSoonest->value;
} // getNextTimer


/**
 * Delete all the timers.  Likely used during a reset.
 */
void timers_deleteAll(duk_context *ctx) {
	list_t *pCurrent = list_first(timerList);
	while (pCurrent != NULL) {
		deleteTimer(ctx, ((timer_record_t *)pCurrent->value)->id);
		pCurrent = list_first(timerList);
	}
} // timers_deleteAll

/**
 * Find the timer record that has the specific id.
 */
static timer_record_t *getById(unsigned long id) {
	list_t *pListRecord = list_first(timerList);
	while(pListRecord != NULL) {
		if (((timer_record_t *)pListRecord->value)->id == id) {
			return (timer_record_t *)pListRecord->value;
		}
		pListRecord = list_next(pListRecord);
	} // End while
	return NULL;
} // getById


/**
 * Clear a previously set interval timer.
 * [0] - number - Interval id.
 */
static duk_ret_t js_timers_clearInterval(duk_context *ctx) {
	/**
	 * We have received a request to clear a previously set interval timer.  This means
	 * that there should be a timer record in the list of timer records scheduled
	 * to fire in the future.  We delete that record.  Now we must post an event
	 * that the timer has cleared.  We do this because there may be some servicing
	 * thread/demon that is waiting for a timer to fire and we want to make sure
	 * that it is informed that the timer has been deleted and shouldn't fire.
	 */
	ESP_LOGD(tag, ">> js_timers_clearInterval");
	duk_int_t timeoutId = duk_get_int(ctx, 0);
	ESP_LOGD(tag, " - Clearing interval timer with id: %d", timeoutId);
	deleteTimer(ctx, timeoutId);
	event_newTimerClearedEvent(timeoutId);
	ESP_LOGD(tag, "<< js_timers_clearInterval");
	return 0;
} // js_timers_clearInterval


/**
 * Clear a previously set timeout.
 * [0] - number - Timeout id.
 */
static duk_ret_t js_timers_clearTimeout(duk_context *ctx) {
	/**
	 * We have received a request to clear a previously set timeout.  This means
	 * that there should be a timer record in the list of timer records scheduled
	 * to fire in the future.  We delete that record.  Now we must post an event
	 * that the timer has cleared.  We do this because there may be some servicing
	 * thread/demon that is waiting for a timer to fire and we want to make sure
	 * that it is informed that the timer has been deleted and shouldn't fire.
	 */
	ESP_LOGD(tag, ">> js_timers_clearTimeout");
	duk_int_t timeoutId = duk_get_int(ctx, 0);
	deleteTimer(ctx, timeoutId);
	event_newTimerClearedEvent(timeoutId);
	ESP_LOGD(tag, "<< js_timers_clearTimeout");
	return 0;
} // js_timers_clearTimeout


/**
 * Create a new timeout or interval timer...
 * [0] - function - Callback function.
 * [1] - number - Delay in milliseconds.
 */
static unsigned long setTimer(duk_context *ctx, bool isInterval) {

	if (!duk_is_object(ctx, 0)) {
		ESP_LOGE(tag, "setTimer: No function passed.");
		return -1;
	}

	if (!duk_is_number(ctx, 1)) {
		ESP_LOGE(tag, "setTimer: No interval passed.");
		return -1;
	}

	unsigned long id = g_nextId++; // Allocate the id for the new timer.

	// We want to add the function that is at ctx[0] to the property of the
	// <heapStash>.timers object with the name of the "id"


	duk_push_heap_stash(ctx);
	// [0] - function
	// [1] - number
	// [2] - heap stash

	duk_get_prop_string(ctx, -1, "timers"); // -3
	// [0] - function
	// [1] - number
	// [2] - heap stash
	// [3] - timers

	duk_push_int(ctx, id);
	// [0] - function
	// [1] - number
	// [2] - heap stash
	// [3] - timers
	// [4] - id


	duk_dup(ctx, 0);
	// [0] - function
	// [1] - number
	// [2] - heap stash
	// [3] - timers (-3)
	// [4] - id (-2)
	// [5] - function (-1)


	duk_put_prop(ctx, -3);
	// [0] - function
	// [1] - number
	// [2] - heap stash
	// [3] - timers


	duk_pop_2(ctx);
	// [0] - function
	// [1] - number

	duk_int_t delayMsecs = duk_get_int(ctx, 1);

	timer_record_t *pTimerRecord = malloc(sizeof(timer_record_t));
	pTimerRecord->id = id;
	pTimerRecord->isInterval = isInterval;
	pTimerRecord->duration.tv_sec = delayMsecs/1000;
	pTimerRecord->duration.tv_usec = (delayMsecs%1000) * 1000;
	gettimeofday(&pTimerRecord->wakeTime, NULL);
	timeval_addMsecs(&pTimerRecord->wakeTime, delayMsecs);

	list_insert(timerList, pTimerRecord); // Insert the timer into the list.

	event_newTimerAddedEvent(id);

	return id;
} // setTimer


/*
 * Set a interval function.
 * [0] - function - Callback function.
 * [1] - number - Delay in milliseconds.
 */
static duk_ret_t js_timers_setInterval(duk_context *ctx) {
	ESP_LOGD(tag, ">> js_timers_setInterval");
	unsigned long id = setTimer(ctx, 1);
	duk_push_number(ctx, id);
	ESP_LOGD(tag, "<< js_timers_setInterval");
	return 1;
} // js_timers_setInterval


/*
 * Set a timeout function.
 * [0] - function - Callback function.
 * [1] - number - Delay in milliseconds.
 */
static duk_ret_t js_timers_setTimeout(duk_context *ctx) {
	ESP_LOGD(tag, ">> js_timers_setTimeout");
	unsigned long id = setTimer(ctx, 0);
	duk_push_number(ctx, id);
	ESP_LOGD(tag, "<< js_timers_setTimeout");
	return 1;
} // js_timers_setTimeout


/**
 * Run the function associated with the timer.
 */
void timers_runTimer(duk_context *ctx, unsigned long id) {
	duk_int_t callRc;
	// Push the heapStash onto the stack.
	// Push the timers object onto the stack.
	// Get the function that is associated with the timers object.
	// Call the function

	duk_push_heap_stash(ctx);
	// [0] - heap stash

	duk_get_prop_string(ctx, -1, "timers");
	// [0] - heap stash
	// [1] - timers

	duk_push_int(ctx, id);
	// [0] - heap stash
	// [1] - timers
	// [2] - key (timer id)


	// Call the function in the timers object which has a property name of "id".
	//HEAP_CHANGE_START();

	callRc = duk_pcall_prop(ctx, -2,
		0); // Number of arguments

	// [0] - heap stash
	// [1] - timers (-2)
	// [2] - retVal (-1)
	if (callRc != 0) {
		ESP_LOGD(tag, "Error detected running timer function!");
		esp32_duktape_log_error(ctx);
	}

	// <empty>
	duk_pop_3(ctx);

	// Get the timer record by its Id.
	timer_record_t *pTimerRecord = getById(id);
	if (pTimerRecord == NULL) {
		ESP_LOGE(tag, "Failed to find timer record in list of records; id=%lu", id);
		return;
	}

	// Now that we have run the timer, it is time to determine the fate of the timer
	// record.  If it is a single shot timeout, then we delete it.  If it is an interval
	// timer which repeats, then we schedule it to fire again by taking the time.
	if (pTimerRecord->isInterval) {
		// There are two possibilities for an interval timer restart time.  The first
		// is "now + duration" and the other is "scheduled wake time + duration".  It
		// isn't clear which we should use.  The "now + duration" will drift from the
		// optimal ... but may be "safer".
		struct timeval now;
		gettimeofday(&now, NULL);
		pTimerRecord->wakeTime = timeval_add(&now, &pTimerRecord->duration);
		//pTimerRecord->wakeTime = timeval_add(&pTimerRecord->wakeTime, &pTimerRecord->duration);
	} else {
		// Since we were not an interval timer and hence were a timeout timer, we have
		// had our one shot at running and we can now be deleted.
		deleteTimer(ctx, id);
	}
} // timers_runTimer


/**
 * Create the Timers functions in Global and initialize the
 * timers environment.
 */
void ModuleTIMERS(duk_context *ctx) {
	// If the list is not already empty, empty it now.
	if (timerList != NULL) {
		list_deleteList(timerList, 1 /* with free */);
	}
	timerList = list_createList();

	// [0] - Global object
	duk_push_global_object(ctx);

	// [0] - Global object
	// [1] - C function - js_timers_clearInterval
	duk_push_c_function(ctx, js_timers_clearInterval, 1);

	// [0] - Global object
	duk_put_prop_string(ctx, -2, "clearInterval"); // Add clearInterval to global

	// [0] - Global object
	// [1] - C function - js_timers_clearTimeout
	duk_push_c_function(ctx, js_timers_clearTimeout, 1);

	// [0] - Global object
	duk_put_prop_string(ctx, -2, "clearTimeout"); // Add clearTimeout to global

	// [0] - Global object
	// [1] - C function - js_timers_setInterval
	duk_push_c_function(ctx, js_timers_setInterval, 2);

	// [0] - Global object
	duk_put_prop_string(ctx, -2, "setInterval"); // Add setInterval to global

	// [0] - Global object
	// [1] - C function - js_timers_setTimeout
	duk_push_c_function(ctx, js_timers_setTimeout, 2);

	// [0] - Global object
	duk_put_prop_string(ctx, -2, "setTimeout"); // Add setTimeout to global

	duk_pop(ctx); // Pop the global object

	// Create the empty timers object in the heap stash.
	// [0] - heap stash
	duk_push_heap_stash(ctx); // Push the heap stash onto the value stack.

	// [0] - heap stash
	// [1] - new object
	duk_push_object(ctx); // Push a new object onto the value stack.

	// [0] - heap stash
	duk_put_prop_string(ctx, -2, "timers"); // Set the new object as "timers" in the heap stash object.

	duk_pop(ctx); // Pop the heap stash object
} // ModuleTIMERS
