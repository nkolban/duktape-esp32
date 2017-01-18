#include <freertos/FreeRTOS.h>
#include <freertos/heap_regions.h>
#include <freertos/task.h>
#include <esp_event.h>
#include <esp_event_loop.h>
#include <esp_log.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <heap_alloc_caps.h>
#include <nvs_flash.h>

#include "duktape_task.h"
#include "duktape_event.h"
#include "logging.h"
#include "sdkconfig.h"


LOG_TAG("duktape_main");

/**
 * Receive data from the telnet or uart and process it.
 */
/*
static void recvData(uint8_t *buffer, size_t size) {
	LOGD("We received: %.*s", size, buffer);
	// We have received a line of data from the telnet client, now we want
	// to present it to Duktape for processing.  We do this by creating an event
	// and placing it on the event processing queue.  The type of the event will be
	// ESP32_DUKTAPE_EVENT_COMMAND_LINE which will contain the data and length.
	// The data will eventually have to be released.

	event_newCommandLineEvent((char *)buffer, size,
	1); // 1 = frome keyboard
} // recvData

*/

/*
static void newTelnetPartner() {
	duktape_init_environment();
} // newTelnetPartner
*/

/**
 * Process telnet commands on a separate task.
 */
/*
static void telnetTask(void *data) {
	LOGD(">> telnetTask");
	telnet_esp32_listenForClients(recvData, newTelnetPartner);
	LOGD("<< telnetTask");
	vTaskDelete(NULL);
} // newTelnetPartner

*/

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
	return ESP_OK;
} // esp32_wifi_eventHandler

void socket_server(void *ignore);

static void init() {
	// At this point we should have a WiFi network
	//esp_event_loop_set_cb(esp32_wifi_eventHandler, NULL);
	esp32_duktape_initEvents();
	//setupVFS(); // Setup the Virtual File System.
	//setupWebVFS("/web", "http://192.168.1.105");
	//xTaskCreatePinnedToCore(&telnetTask, "telnetTask", 8048, NULL, 5, NULL, 0);
	//startMongooseServer();
	//xTaskCreatePinnedToCore(&socket_server, "socket_server", 8048, NULL, 5, NULL, 0);
	xTaskCreatePinnedToCore(&duktape_task, "duktape_task", 16*1024, NULL, 5, NULL, tskNO_AFFINITY);
} // init


/*
 *  Main entry point into the application.
 */
void app_main(void)
{
	LOGD("Free heap at start: %d", esp_get_free_heap_size());
	LOGD("Free IRAM: %d",  xPortGetFreeHeapSizeTagged(MALLOC_CAP_32BIT));
	// Boot the WiFi environment and once WiFi is ready, call init().
	nvs_flash_init();
	tcpip_adapter_init();
	ESP_ERROR_CHECK(esp_event_loop_init(esp32_wifi_eventHandler, NULL));
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
	//bootWiFi(init);
	init();
} // app_main
