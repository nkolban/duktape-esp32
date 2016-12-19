#ifndef _LOGGING_H
#define _LOGGING_H

#define LOG_TAG(_TAG) static char tag[] = _TAG

#ifdef ESP_PLATFORM
#include <esp_log.h>
#include "sdkconfig.h"

#define LOGV(...) ESP_LOGV(tag, __VA_ARGS__)
#define LOGE(...) ESP_LOGE(tag, __VA_ARGS__)
#define LOGD(...) ESP_LOGD(tag, __VA_ARGS__)
#define LOGW(...) ESP_LOGW(tag, __VA_ARGS__)

#else
extern void dukf_log(char *tag, char type, char *fmt, ...);
#define LOGV(...) dukf_log(tag, 'V', __VA_ARGS__)
#define LOGE(...) dukf_log(tag, 'E', __VA_ARGS__)
#define LOGD(...) dukf_log(tag, 'D', __VA_ARGS__)
#define LOGW(...) dukf_log(tag, 'W', __VA_ARGS__)
/*
#define LOGV(...)  { \
	printf("(V) %s: ", tag); \
	printf(__VA_ARGS__); \
	printf("\n"); \
}
#define LOGE(...)  { \
	printf("(E) %s: ", tag); \
	printf(__VA_ARGS__); \
	printf("\n"); \
}
#define LOGD(...)  { \
	printf("(D) %s: ", tag); \
	printf(__VA_ARGS__); \
	printf("\n"); \
}
#define LOGW(...)  { \
	printf("(W) %s: ", tag); \
	printf(__VA_ARGS__); \
	printf("\n"); \
}
*/
#endif
#endif
