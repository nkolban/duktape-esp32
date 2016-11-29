#include <freertos/FreeRTOS.h>
#include <esp_wifi.h>
#include <esp_system.h>
#include <esp_event.h>
#include <esp_event_loop.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <lwip/sockets.h>
#include <string.h>
#include "bootwifi.h"
#include "esp32_specific.h"
#include "telnet.h"
#include "esp32_duktape/duktape_event.h"
#include "esp32_mongoose.h"
#include "connection_info.h"
#include "duktape_task.h"
#include "sdkconfig.h"

char tag[] = "duktape_main";

/**
 * Receive data from the telnet or uart and process it.
 */
static void recvData(uint8_t *buffer, size_t size) {
	ESP_LOGD(tag, "We received: %.*s", size, buffer);
	// We have received a line of data from the telnet client, now we want
	// to present it to Duktape for processing.  We do this by creating an event
	// and placing it on the event processing queue.  The type of the event will be
	// ESP32_DUKTAPE_EVENT_COMMAND_LINE which will contain the data and length.
	// The data will eventually have to be released.

	event_newCommandLineEvent((char *)buffer, size, 1 /* from keyboard */);
} // recvData

static void newTelnetPartner() {
	duktape_init_environment();
} // newTelnetPartner

/**
 * Process telnet commands on a separate task.
 */
static void telnetTask(void *data) {
	ESP_LOGD(tag, ">> telnetTask");
	telnet_esp32_listenForClients(recvData, newTelnetPartner);
	ESP_LOGD(tag, "<< telnetTask");
	vTaskDelete(NULL);
} // newTelnetPartner

/**
 * An ESP32 WiFi event handler.
 * The types of events that can be received here are:
 *
 * SYSTEM_EVENT_AP_PROBEREQRECVED
 * SYSTEM_EVENT_AP_STACONNECTED
 * SYSTEM_EVENT_AP_STADISCONNECTED
 * SYSTEM_EVENT_AP_START
 * SYSTEM_EVENT_AP_STOP
 * SYSTEM_EVENT_SCAN_DONE
 * SYSTEM_EVENT_STA_AUTHMODE_CHANGE
 * SYSTEM_EVENT_STA_CONNECTED
 * SYSTEM_EVENT_STA_DISCONNECTED
 * SYSTEM_EVENT_STA_GOT_IP
 * SYSTEM_EVENT_STA_START
 * SYSTEM_EVENT_STA_STOP
 * SYSTEM_EVENT_WIFI_READY
 */
static esp_err_t esp32_wifi_eventHandler(void *ctx, system_event_t *event) {
	// Your event handling code here...
	return ESP_OK;
}

static void init() {
	// At this point we should have a WiFi network
	esp_event_loop_set_cb(esp32_wifi_eventHandler, NULL);
	esp32_duktape_initEvents();
	setupVFS(); // Setup the Virtual File System.
	//setupWebVFS("/web", "http://192.168.1.105");
	xTaskCreatePinnedToCore(&telnetTask, "telnetTask", 8048, NULL, 5, NULL, 0);
	startMongooseServer();
	xTaskCreatePinnedToCore(&duktape_task, "duktape_task", 10*1024, NULL, 5, NULL, 0);
} // init

/**
 * Main entry point into the application.
 */
void app_main(void)
{
	bootWiFi(init);
} // app_main

/**
 * Hack fix for ESP-IDF issue https://github.com/espressif/esp-idf/issues/83
 */
double __ieee754_remainder(double x, double y) {
	return x - y * floor(x/y);
} // __ieee754_remainder
