#include "logging.h"
#ifndef ESP_PLATFORM
#include <stdio.h>
#include <stdarg.h>
#include <sys/time.h>
#include <stdio.h>
#include "c_timeutils.h"

static struct timeval startTime;

// Have the initi function called at startup.
static void init() __attribute__ ((constructor));
static void init() {
	gettimeofday(&startTime, NULL);
} // init

/**
 * Write a log message to stdout.  The function takes the "type" of the
 * message as a single character which is likely to be 'E' for Error, 'W' for
 * Warning, 'D' for Debug and 'V' for Verbose.  Then there is a printf format
 * string followed by parameters.  The output is of the same form as ESP32 logging.
 */
void dukf_log(char *tag, char type, char *fmt, ...) {
	struct timeval now;
	gettimeofday(&now, NULL);
	struct timeval diff_tv = timeval_sub(&now, &startTime);
	uint32_t diff = timeval_toMsecs(&diff_tv);
	printf("%c (%d) %s: ", type, diff, tag);
	va_list vl;
	va_start(vl, fmt);
	vprintf(fmt, vl);
	va_end(vl);
	printf("\n");
}
#endif
