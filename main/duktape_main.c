#include <freertos/FreeRTOS.h>
#include <esp_wifi.h>
#include <esp_system.h>
#include <esp_event.h>
#include <esp_event_loop.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <lwip/sockets.h>
#include <string.h>
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

	newCommandLineEvent((char *)buffer, size, 1 /* from keyboard */);
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
 * WiFi Event handler
 */
static esp_err_t wifiEventHandler(void *ctx, system_event_t *event)
{
	// We have got an IP address!!
	if (event->event_id == SYSTEM_EVENT_STA_GOT_IP) {
		xTaskCreatePinnedToCore(&telnetTask, "telnetTask", 8048, NULL, 5, NULL, 0);
		startMongooseServer();
	}
  return ESP_OK;
} // wifiEventHandler

static void init() {
	esp32_duktape_initEvents();
}

/**
 * Main entry point into the application.
 */
void app_main(void)
{
	nvs_flash_init();
	tcpip_adapter_init();

	tcpip_adapter_dhcpc_stop(TCPIP_ADAPTER_IF_STA); // Don't run a DHCP client
	tcpip_adapter_ip_info_t ipInfo;

	inet_pton(AF_INET, DEVICE_IP, &ipInfo.ip);
	inet_pton(AF_INET, DEVICE_GW, &ipInfo.gw);
	inet_pton(AF_INET, DEVICE_NETMASK, &ipInfo.netmask);
	tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_STA, &ipInfo);

	init();
	ESP_ERROR_CHECK(esp_event_loop_init(wifiEventHandler, NULL));
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg) );
	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	wifi_config_t sta_config = {
		.sta = {
			.ssid      = AP_TARGET_SSID,
			.password  = AP_TARGET_PASSWORD,
			.bssid_set = 0
		}
	};
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_config));
	ESP_ERROR_CHECK(esp_wifi_start());
	ESP_ERROR_CHECK(esp_wifi_connect());

	setupVFS(); // Setup the Virtual File System.
	setupWebVFS("/web", "http://192.168.1.105");
	xTaskCreatePinnedToCore(&duktape_task, "duktape_task", 10*1024, NULL, 5, NULL, 0);
} // app_main

double __ieee754_remainder(double x, double y) {
	return 0;
}
