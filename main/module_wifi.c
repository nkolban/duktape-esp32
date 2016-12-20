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

#include <duktape.h>
#include <esp_err.h>
#include <esp_event_loop.h>
#include <esp_log.h>
#include <esp_wifi.h>
#include <lwip/dns.h>
#include <lwip/sockets.h>

#include "duktape_utils.h"
#include "esp32_duktape/duktape_event.h"
#include "esp32_duktape/module_wifi.h"
#include "logging.h"
#include "sdkconfig.h"

LOG_TAG("module_wifi");

static uint32_t g_scanCallbackStashKey = -1;
static uint32_t g_gotipCallbackStashKey = -1;
static uint32_t g_apstartCallbackStashKey = -1;

/**
 * Convert an authentication mode to a string.
 */
static char *authModeToString(wifi_auth_mode_t mode) {
	switch(mode) {
	case WIFI_AUTH_OPEN:
		return "open";
	case WIFI_AUTH_WEP:
		return "wep";
	case WIFI_AUTH_WPA_PSK:
		return "wpa";
	case WIFI_AUTH_WPA2_PSK:
		return "wpa2";
	case WIFI_AUTH_WPA_WPA2_PSK:
		return "wpa_wpa2";
	default:
		return "unknown";
	}
} // authModeToString


// Handle the event that a Wifi scan has completed.  Our logic here is to build an
// object that contains the results and leave that on the value stack.
//
// A published scan record will contain:
// {
//    ssid: <network id>,
//    mac: <mac address>
//    rssi: <signal strength>
//    auth: <authentication mode>
// }
//
static int scanParamsDataProvider(duk_context *ctx, void *context) {
	LOGD("Here we build the scan results ...");


	LOGD("Process a scan result event.");

	// Get the number of access points in our last scan.
	uint16_t numAp;
	ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&numAp));

	// Allocate storage for our scan results and retrieve them.
	wifi_ap_record_t *apRecords = calloc(sizeof(wifi_ap_record_t), numAp);
	ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&numAp, apRecords));
	LOGD("Number of Access Points: %d", numAp);

	// Build an array for the results ...
	duk_idx_t resultIdx = duk_push_array(ctx);
	// [0] - Array - results

	// Add each of the scan results into the array.
	int i;
	for (i=0; i<numAp; i++) {
		LOGD("%d: ssid=%s", i, apRecords[i].ssid);
		duk_idx_t scanResultIdx = duk_push_object(ctx); // Create a new scan result object
		// [0] - Array - results
		// [1] - New object

		duk_push_string(ctx, (char *)apRecords[i].ssid);
		// [0] - Array - results
		// [1] - New object
		// [2] - String (ssid)

		duk_put_prop_string(ctx, scanResultIdx, "ssid");
		// [0] - Array - results
		// [1] - New object

		duk_push_sprintf(ctx, MACSTR, MAC2STR(apRecords[i].bssid));
		// [0] - Array - results
		// [1] - New object
		// [2] - String (mac)

		duk_put_prop_string(ctx, scanResultIdx, "mac");
		// [0] - Array - results
		// [1] - New object

		duk_push_int(ctx, apRecords[i].rssi);
		// [0] - Array - results
		// [1] - New object
		// [2] - Number (rssi)

		duk_put_prop_string(ctx, scanResultIdx, "rssi");
		// [0] - Array - results
		// [1] - New object

		duk_push_string(ctx, authModeToString(apRecords[i].authmode));
		// [0] - Array - results
		// [1] - New object
		// [2] - String (auth)

		duk_put_prop_string(ctx, scanResultIdx, "auth");
		// [0] - Array - results
		// [1] - New object

		duk_put_prop_index(ctx, resultIdx, i); // Add the new record into the results array
		// [0] - Array - results
	}
	free(apRecords);
	return 1;
} //scanParamsDataProvider


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
	LOGD(">> esp32_wifi_eventHandler");
	// Your event handling code here...
	switch(event->event_id) {
		case SYSTEM_EVENT_STA_GOT_IP: {
			if (g_gotipCallbackStashKey == -1) {
				break;
			}
			event_newCallbackRequestedEvent(
				ESP32_DUKTAPE_CALLBACK_TYPE_FUNCTION,
				g_gotipCallbackStashKey,
				NULL, // No data provider function
				NULL // No context data
			);
			g_gotipCallbackStashKey = -1;
			break;
		}

		case SYSTEM_EVENT_AP_START: {
			if (g_gotipCallbackStashKey == -1) {
				break;
			}
			event_newCallbackRequestedEvent(
				ESP32_DUKTAPE_CALLBACK_TYPE_FUNCTION,
				g_apstartCallbackStashKey,
				NULL, // No data provider function
				NULL // No context data
			);
			g_apstartCallbackStashKey = -1;
			break;
		}

		case SYSTEM_EVENT_SCAN_DONE: {
			// If we have received a scan done but no callback waiting, do nothing.
			if (g_scanCallbackStashKey == -1) {
				break;
			}
			// We now have a scan ... so let us now raise an event saying that
			// the scan has completed!
			//event_newWifiScanCompletedEvent();
			event_newCallbackRequestedEvent(
				ESP32_DUKTAPE_CALLBACK_TYPE_FUNCTION,
				g_scanCallbackStashKey,
				scanParamsDataProvider,
				NULL // No context data
			);
			g_scanCallbackStashKey = -1;
			break;
		}

		default: {
			break;
		}
	}
	LOGD("<< esp32_wifi_eventHandler");
	return ESP_OK;
} // esp32_wifi_eventHandler


/**
 * Handle a request to do a scan.  We are expecting that the caller will pass
 * in a function reference that will be a callback function.
 * [0] - Function object
 */
static duk_ret_t js_wifi_scan(duk_context *ctx) {
	LOGD(">> js_wifi_scan");
	if (!duk_is_function(ctx, 0)) {
		LOGD("Scan: not a function!");
		return 0;
	}

	// Stash the top [1] items on the value stack and place in an array in stash.
	g_scanCallbackStashKey = esp32_duktape_stash_array(ctx, 1);

	/*
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
	 */

	// Now that we have saved the callback, request that the scan is performed.
	wifi_scan_config_t conf;
	conf.channel     = 0;
	conf.bssid       = NULL;
	conf.ssid        = NULL;
	conf.show_hidden = 1;
	ESP_ERROR_CHECK(esp_wifi_scan_start(&conf, 0 /* don't block */));

	LOGD("<< js_wifi_scan");
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

	LOGD(">> js_wifi_getDNS");

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

	LOGD("<< js_wifi_getDNS");
	return 1; // New array - DNS servers
} // js_wifi_getDNS


/**
 * Connect WiFi to an access point.
 * options:
 * - ssid
 * - password [optional]
 * - network [optional]
 *  - ip
 *  - gw
 *  - netmask
 * callback: a callback function
 */
static duk_ret_t js_wifi_connect(duk_context *ctx) {
	LOGD(">> js_wifi_connect");
	char *password = ""; // Default password is none.
  tcpip_adapter_ip_info_t ipInfo;
  duk_idx_t optionsIdx = 0;


  if (!duk_is_object(ctx, -2)) {
  	duk_error(ctx, 1, "Missing options object");
  }

  // [0] - Options
  // [1] - Callback | Undefined [optional]
  if (duk_is_function(ctx, -1)) {
  	// we have a callback
  	g_gotipCallbackStashKey = esp32_duktape_stash_array(ctx, 1);
  } else {
  	duk_pop(ctx); // Remove the thing that might have been a callback function
  }

	duk_get_prop_string(ctx, optionsIdx, "ssid");
	if (!duk_is_string(ctx, -1)) {
		duk_error(ctx, 1, "Invalid ssid");
	}
	const char *ssid = duk_get_string(ctx, -1);
	duk_pop(ctx);

	// The password is optional.
	if (duk_has_prop_string(ctx, optionsIdx, "password")) {
		duk_get_prop_string(ctx, -1, "password");
		if (!duk_is_string(ctx, -1)) {
			duk_error(ctx, 1, "Invalid password");
		}
		password = (char *)duk_get_string(ctx, -1);
		duk_pop(ctx);
	}

	ESP_ERROR_CHECK(esp_wifi_disconnect());

	// Handle any network options that may have been supplied.  If network options
	// were supplied then we will NOT be using DHCP and instead we should have
	// been supplied our own IP address, gateway address and netmask.  Extract
	// and use those.
	if (duk_has_prop_string(ctx, optionsIdx, "network")) {

		duk_get_prop_string(ctx, -1, "network");
		duk_get_prop_string(ctx, -1, "ip");
		const char *ipString = duk_get_string(ctx, -1);
		duk_pop(ctx);

		duk_get_prop_string(ctx, -1, "gw");
		const char *gwString = duk_get_string(ctx, -1);
		duk_pop(ctx);

		duk_get_prop_string(ctx, -1, "netmask");
		const char *netmaskString = duk_get_string(ctx, -1);
		duk_pop(ctx);

	  inet_pton(AF_INET, ipString, &ipInfo.ip);
	  inet_pton(AF_INET, gwString, &ipInfo.gw);
	  inet_pton(AF_INET, netmaskString, &ipInfo.netmask);

	  LOGD(" - Using specific network info: ip: %s, gw: %s, netmask: %s",
	  	ipString, gwString, netmaskString);
	  tcpip_adapter_dhcpc_stop(TCPIP_ADAPTER_IF_STA); // Don't run a DHCP client
	  tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_STA, &ipInfo);

	} else {
		// Since we were NOT supplied network information, use DHCP.
	  ESP_ERROR_CHECK(tcpip_adapter_dhcpc_start(TCPIP_ADAPTER_IF_STA));
	}

	// Perform the actual connection to the access point.
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	LOGD(" - Connecting to access point: %s with %s", ssid, password);
  wifi_config_t sta_config;
  strcpy(sta_config.sta.ssid, ssid);
  strcpy(sta_config.sta.password, password);
  sta_config.sta.bssid_set = 0;
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_config));
  ESP_ERROR_CHECK(esp_wifi_start());
  ESP_ERROR_CHECK(esp_wifi_connect());
  return 0;
} // js_wifi_connect


/**
 * Listen as an access point
 * options:
 * - ssid
 * - password
 * - auth
 * callback function: [optional]
 */
static duk_ret_t js_wifi_listen(duk_context *ctx) {
	LOGD(">> js_wifi_listen");

  if (!duk_is_object(ctx, -2)) {
  	duk_error(ctx, 1, "Missing options object");
  }

  // [0] - Options
  // [1] - Callback | Undefined [optional]
  if (duk_is_function(ctx, -1)) {
  	// we have a callback
  	g_gotipCallbackStashKey = esp32_duktape_stash_array(ctx, 1);
  } else {
  	duk_pop(ctx); // Remove the thing that might have been a callback function
  }

	char *password = "";

	duk_get_prop_string(ctx, -1, "ssid");
	if (!duk_is_string(ctx, -1)) {
		duk_error(ctx, 1, "Invalid ssid");
	}
	const char *ssid = duk_get_string(ctx, -1);
	duk_pop(ctx);

	duk_get_prop_string(ctx, -1, "auth");
	if (!duk_is_string(ctx, -1)) {
		duk_error(ctx, 1, "Invalid auth");
	}
	const char *auth = duk_get_string(ctx, -1);
	duk_pop(ctx);

	wifi_auth_mode_t authMode;
	if (strcmp(auth, "open") == 0) {
		authMode = WIFI_AUTH_OPEN;
	}
	else if (strcmp(auth, "wep") == 0) {
		authMode = WIFI_AUTH_WEP;
	}
	else if (strcmp(auth, "wpa") == 0) {
		authMode = WIFI_AUTH_WPA_PSK;
	}
	else if (strcmp(auth, "wpa2") == 0) {
		authMode = WIFI_AUTH_WPA2_PSK;
	}
	else if (strcmp(auth, "wpa_wpa2") == 0) {
		authMode = WIFI_AUTH_WPA_WPA2_PSK;
	} else {
		duk_error(ctx, 1, "Invalid auth: %s", auth);
	}

	if (authMode != WIFI_AUTH_OPEN) {
		duk_get_prop_string(ctx, -1, "password");
		if (!duk_is_string(ctx, -1)) {
			duk_error(ctx, 1, "Invalid password");
		}
		password = (char *)duk_get_string(ctx, -1);
		duk_pop(ctx);
	}

	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
	LOGD(" - Being an access point: %s with auth mode %s", ssid, auth);
	wifi_config_t ap_config;
	strcpy(ap_config.ap.ssid, ssid);
	ap_config.ap.ssid_len = 0;
	strcpy(ap_config.ap.password, password);
	ap_config.ap.channel = 0;
	ap_config.ap.authmode = authMode;
	ap_config.ap.ssid_hidden = 0;
	ap_config.ap.max_connection = 4;
	ap_config.ap.beacon_interval = 100;
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
	ESP_ERROR_CHECK(esp_wifi_connect());
	return 0;
} // js_wifi_listen


/**
 * Create the WIFI module in Global.
 */
void ModuleWIFI(duk_context *ctx) {
	duk_push_global_object(ctx);
	// [0] - Global object


	duk_idx_t idx = duk_push_object(ctx); // Create new WIFI object
	// [0] - Global object
	// [1] - New object


	duk_push_c_function(ctx, js_wifi_scan, 1);
	// [0] - Global object
	// [1] - New object
	// [2] - c-func - js_wifi_scan


	duk_put_prop_string(ctx, idx, "scan"); // Add scan to new WIFI
	// [0] - Global object
	// [1] - New object


	duk_push_c_function(ctx, js_wifi_getDNS, 0);
	// [0] - Global object
	// [1] - New object
	// [2] - c-func - js_wifi_getDNS


	duk_put_prop_string(ctx, idx, "getDNS"); // Add getDNS to new WIFI
	// [0] - Global object
	// [1] - New object


	duk_push_c_function(ctx, js_wifi_connect, 2);
	// [0] - Global object
	// [1] - New object
	// [2] - c-func - js_wifi_connect


	duk_put_prop_string(ctx, idx, "connect"); // Add connect to new WIFI
	// [0] - Global object
	// [1] - New object

	duk_push_c_function(ctx, js_wifi_listen, 2);
	// [0] - Global object
	// [1] - New object
	// [2] - c-func - js_wifi_listen


	duk_put_prop_string(ctx, idx, "listen"); // Add listen to new WIFI
	// [0] - Global object
	// [1] - New object


	duk_put_prop_string(ctx, 0, "WIFI"); // Add WIFI to global
	// [0] - Global object

	duk_pop(ctx);

	// Setup the new Event handler
	esp_event_loop_set_cb(esp32_wifi_eventHandler, NULL); // No return code to check.
} // ModuleWIFI
