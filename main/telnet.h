#if !defined(_TELNET_H)
#define _TELNET_H
#include <sys/types.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

typedef struct {
	uint8_t *data;
	ssize_t length;
} line_record_t;

extern QueueHandle_t line_records_queue;
void telnet_esp32_sendData(uint8_t *buffer, size_t size);
int telnet_esp32_vprintf(const char *fmt, va_list va);
void telnet_esp32_listenForClients(
	void (*receivedDatacallbackParam)(uint8_t *buffer, size_t size),
	void (*newTelnetPartnerCallbackParam)());
#endif
