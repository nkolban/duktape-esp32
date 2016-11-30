/**
 * Handle WiFi functions for ESP32-Duktape.  Functions included are:
 *
 * * WIFI.scan(callback) - Scan for WiFi access points and when complete, invoke the callback with
 * an array of found access points.  Each record in the list of access points will include:
 *   * ssid
 *   * rssi
 *   * mac
 *
 * * WIFI.getDNS() - Return an array of two items which are the two IP addresses (as strings)
 * of the DNS servers that we may know about.
 */
#include "esp32_duktape/module_wifi.h"
#include <esp_wifi.h>
#include <duktape.h>
#include <lwip/sockets.h>
#include <lwip/dns.h>
#include <esp_err.h>
#include <esp_log.h>
#include <esp_event_loop.h>
#include "duktape_utils.h"
#include "esp32_duktape/duktape_event.h"
#include "sdkconfig.h"

static char tag[] = "module_wifi";

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
static esp_err_t esp32_wifi_eventHandler(void *param_ctx, system_event_t *event) {
	ESP_LOGD(tag, ">> esp32_wifi_eventHandler");
	// Your event handling code here...
	switch(event->event_id) {
		case SYSTEM_EVENT_SCAN_DONE: {
			// We now have a scan ... so let us now raise an event saying that
			// the scan has completed!
			event_newWifiScanCompletedEvent();
			break;
		}

		default: {
			break;
		}
	}
	return ESP_OK;
} // esp32_wifi_eventHandler


/**
 * Handle a request to do a scan.  We are expecting that the caller will pass
 * in a function reference that will be a callback function.
 * [0] - Function object
 */
static duk_ret_t js_wifi_scan(duk_context *ctx) {
	ESP_LOGD(tag, ">> js_wifi_scan");
	if (!duk_is_function(ctx, 0)) {
		ESP_LOGD(tag, "Scan: not a function!");
		return 0;
	}

	duk_push_heap_stash(ctx);
	// [0] - Function object
	// [1] - Heap stash

	if (!duk_get_prop_string(ctx, -1, "scan_callbacks_array")) {
		// [0] - Function object
		// [1] - Heap stash
		// [2] - scan_callbacks array (undefined)

		duk_pop(ctx);
		// [0] - Function object
		// [1] - Heap stash

		duk_push_array(ctx);
		// [0] - Function object
		// [1] - Heap stash
		// [2] - new array

		duk_put_prop_string(ctx, -2, "scan_callbacks_array");
		// [0] - Function object
		// [1] - Heap stash

		duk_get_prop_string(ctx, -1, "scan_callbacks_array");
		// [0] - Function object
		// [1] - Heap stash
		// [2] - scan_callbacks array
	}

	int length = duk_get_length(ctx, -1);

	duk_dup(ctx, 0);
	// [0] - Function object
	// [1] - Heap stash
	// [2] - scan_callbacks array
	// [3] - Function object

	duk_put_prop_index(ctx, -2, length); // Add the function object to the array.
	// [0] - Function object
	// [1] - Heap stash
	// [2] - scan_callbacks array

	duk_pop_3(ctx);
	// <Stack Empty>

	// Now that we have saved the callback, request that the scan is performed.
	wifi_scan_config_t conf;
	conf.channel     = 0;
	conf.bssid       = NULL;
	conf.ssid        = NULL;
	conf.show_hidden = 1;
	ESP_ERROR_CHECK(esp_wifi_scan_start(&conf, 0 /* don't block */));

	ESP_LOGD(tag, "<< js_wifi_scan");
	return 0;
} // js_wifi_scan


/**
 * Retrieve the DNS servers that are associated with the device.
 * The return is an object on the stack of the form:
 * [ <string>, <string>, ... ] where each of the strings is an IP address
 * of a DNS server.  The reason there are "DNS_MAX_SERVER" is that the ESP32 can have
 * a primary and secondary DNS server (and maybe more or less).
 */
static duk_ret_t js_wifi_getDNS(duk_context *ctx) {
	char ipString[20];
	ip_addr_t ip;
	int i;

	ESP_LOGD(tag, ">> js_wifi_getDNS");

	duk_idx_t idx = duk_push_array(ctx); // Create new WIFI object
	// [0] - New array - DNS servers

	// There are "DNS_MAX_SERVERS" DNS servers that may be known to us.
	for (i=0; i<DNS_MAX_SERVERS; i++) { // DNS_MAX_SERVERS comes from lwip ... see http://www.nongnu.org/lwip/2_0_0/group__dns.html

		ip = dns_getserver(i);
		inet_ntop(AF_INET, &ip, ipString, sizeof(ipString));

		duk_push_string(ctx, ipString);
		// [0] - New array - DNS servers
		// [1] - IP address

		duk_put_prop_index(ctx, idx, 0);
		// [0] - New array - DNS servers
	}

	ESP_LOGD(tag, "<< js_wifi_getDNS");
	return 1; // New array - DNS servers
} // js_wifi_getDNS


/**
 * Create the WIFI module in Global.
 */
void ModuleWIFI(duk_context *ctx) {

	// [0] - Global object
	duk_push_global_object(ctx);

	// [0] - Global object
	// [1] - New object
	duk_idx_t idx = duk_push_object(ctx); // Create new WIFI object

	// [0] - Global object
	// [1] - New object
	// [2] - c-func - js_wifi_scan
	duk_push_c_function(ctx, js_wifi_scan, 1);

	// [0] - Global object
	// [1] - New object
	duk_put_prop_string(ctx, idx, "scan"); // Add scan to new WIFI

	// [0] - Global object
	// [1] - New object
	// [2] - c-func - js_wifi_scan
	duk_push_c_function(ctx, js_wifi_getDNS, 0);

	// [0] - Global object
	// [1] - New object
	duk_put_prop_string(ctx, idx, "getDNS"); // Add getDNS to new WIFI

	// [0] - Global object
	duk_put_prop_string(ctx, 0, "WIFI"); // Add WIFI to global

	duk_pop(ctx);

	// Setup the new Event handler
	esp_event_loop_set_cb(esp32_wifi_eventHandler, NULL); // No return code to check.
} // ModuleWIFI
