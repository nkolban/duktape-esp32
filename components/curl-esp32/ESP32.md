#ESP32
This is a project that is attempting to port libcurl to an ESP32
environment.  To use in your project, edit the `component.mk`
and change the entry that points to my directory path of where we have
downloaded the code to your directory path.

This is specified by the variable called `CURL_DIR`.

The project should be installed in an app template under a 
directory called `components`.  A sample `main.c` application
would be:


```
#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "curl/curl.h"

#define SSID "guest"
#define PASSWORD "kolbanguest"

static char tag[] = "main";

static size_t writeData(void *buffer, size_t size, size_t nmemb, void *userp) {
	ESP_LOGI(tag, "writeData called: buffer=0x%lx, size=%d, nmemb=%d", (unsigned long)buffer, size, nmemb);
	ESP_LOGI(tag, "data>> %.*s", size*nmemb, (char *)buffer);
	return size * nmemb;
}

void testCurl(void *data) {
	CURL *handle;
	CURLcode res;

	// Create a curl handle
	handle = curl_easy_init();
	if (handle == NULL) {
		ESP_LOGI(tag, "Failed to create a curl handle");
		vTaskDelete(NULL);
		return;
	}

	ESP_LOGI(tag, "Created a curl handle ...");

  curl_easy_setopt(handle, CURLOPT_URL, "http://example.com");
  /* example.com is redirected, so we tell libcurl to follow redirection */
  curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, writeData);
  /* Perform the request, res will get the return code */
  res = curl_easy_perform(handle);
  if (res != CURLE_OK) {
  	ESP_LOGI(tag, "curl_easy_perform failed: %s", curl_easy_strerror(res));
  } else {
  	ESP_LOGI(tag, "curl_easy_perform() completed without incident.");
  }
  curl_easy_cleanup(handle);
	vTaskDelete(NULL);
} // End of testCurl


esp_err_t event_handler(void *ctx, system_event_t *event)
{
	if (event->event_id == SYSTEM_EVENT_STA_GOT_IP) {
		ESP_LOGI(tag, "Got an IP ... ready to go!");
		xTaskCreatePinnedToCore(&testCurl, "testCurl", 10000, NULL, 5, NULL, 0);
		return ESP_OK;
	}
  return ESP_OK;
}

int app_main(void)
{
    nvs_flash_init();
    system_init();
    tcpip_adapter_init();
    ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    wifi_config_t sta_config = {
        .sta = {
            .ssid = SSID,
            .password = PASSWORD,
            .bssid_set = 0
        }
    };
    ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_STA, &sta_config) );
    ESP_ERROR_CHECK( esp_wifi_start() );
    ESP_ERROR_CHECK( esp_wifi_connect() );

    return 0;
}
```
