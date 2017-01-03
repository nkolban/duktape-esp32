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
#include "duktape_event.h"
#include "esp32_specific.h"
#include "logging.h"
#include "module_wifi.h"
#include "sdkconfig.h"

LOG_TAG("module_wifi");

static uint32_t g_scanCallbackStashKey = -1;
static uint32_t g_gotipCallbackStashKey = -1;
static uint32_t g_apStartCallbackStashKey = -1;

static int scanParamsDataProvider(duk_context *ctx, void *context);

static int addError(duk_context *ctx, void *context) {
	if (context == NULL) {
		duk_push_null(ctx);
	} else {
		duk_push_string(ctx, (char *)context);
	}
	return 1;
}


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
				addError, // No data provider function
				NULL  // No context data
			);
			g_gotipCallbackStashKey = -1;
			break;
		} // SYSTEM_EVENT_STA_GOT_IP


		case SYSTEM_EVENT_STA_DISCONNECTED: {
			if (g_gotipCallbackStashKey == -1) {
				break;
			}
			event_newCallbackRequestedEvent(
				ESP32_DUKTAPE_CALLBACK_TYPE_FUNCTION,
				g_gotipCallbackStashKey,
				addError, // No data provider function
				"STA_DISCONNECTED"
			);
			g_gotipCallbackStashKey = -1;
			break;
		} // SYSTEM_EVENT_STA_DISCONNECTED


		case SYSTEM_EVENT_AP_START: {
			if (g_apStartCallbackStashKey == -1) {
				LOGD("No AP Start callback stash key, no callback to invoke.");
				break;
			}
			// We have a stashed callback to invoke, so invoke it...
			event_newCallbackRequestedEvent(
				ESP32_DUKTAPE_CALLBACK_TYPE_FUNCTION,
				g_apStartCallbackStashKey,
				NULL, // No data provider function
				NULL  // No context data
			);
			g_apStartCallbackStashKey = -1;
			break;
		} // SYSTEM_EVENT_AP_START


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
		} // SYSTEM_EVENT_SCAN_DONE

		default: {
			break;
		}
	}
	LOGD("<< esp32_wifi_eventHandler");
	return ESP_OK;
} // esp32_wifi_eventHandler


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
	esp_err_t errRc;

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

	/*
	errRc = esp_wifi_disconnect();
	if (errRc != ESP_OK) {
		LOGD("esp_wifi_disconnect rc=%s", esp32_errToString(errRc));
	}
*/
	// Handle any network options that may have been supplied.  If network options
	// were supplied then we will NOT be using DHCP and instead we should have
	// been supplied our own IP address, gateway address and netmask.  Extract
	// and use those.
	int useDHCP = 1;
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

		// We now try and get network IP information but check that it is good first
	  if (inet_pton(AF_INET, ipString, &ipInfo.ip) == 1 &&
	  inet_pton(AF_INET, gwString, &ipInfo.gw) == 1 &&
	  inet_pton(AF_INET, netmaskString, &ipInfo.netmask)) {
			LOGD(" - Using specific network info: ip: %s, gw: %s, netmask: %s",
				ipString, gwString, netmaskString);
			tcpip_adapter_dhcpc_stop(TCPIP_ADAPTER_IF_STA); // Don't run a DHCP client
			tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_STA, &ipInfo);
			useDHCP = 0;
	  } // Setup network portion
	} // We have a network object

	if (useDHCP) {
		// Since we were NOT supplied network information or couldn't use it, use DHCP.
		errRc = tcpip_adapter_dhcpc_start(TCPIP_ADAPTER_IF_STA);
		if (errRc != ESP_OK) {
			duk_error(ctx, 1, "tcpip_adapter_dhcpc_start rc=%s", esp32_errToString(errRc));
		}
	} // useDHCP is true

	// Perform the actual connection to the access point.
	errRc = esp_wifi_set_mode(WIFI_MODE_STA);
	if (errRc != ESP_OK) {
		LOGD("esp_wifi_set_mode() rc=%d", errRc);
		duk_error(ctx, 1, "esp_wifi_set_mode rc=%s", esp32_errToString(errRc));
	}

	LOGD(" - Connecting to access point: \"%s\" with \"%s\"", ssid, "<Password hidden>");
  wifi_config_t sta_config;
  strcpy((char *)sta_config.sta.ssid, ssid);
  strcpy((char *)sta_config.sta.password, password);
  sta_config.sta.bssid_set = 0;

  errRc = esp_wifi_set_config(WIFI_IF_STA, &sta_config);
	if (errRc != ESP_OK) {
		LOGD("esp_wifi_set_config() rc=%d", errRc);
		duk_error(ctx, 1, "esp_wifi_set_config rc=%s", esp32_errToString(errRc));
	}

	errRc = esp_wifi_start();
	if (errRc != ESP_OK) {
		LOGD("esp_wifi_start() rc=%d", errRc);
		duk_error(ctx, 1, "esp_wifi_start rc=%s", esp32_errToString(errRc));
	}

	errRc = esp_wifi_connect();
	if (errRc != ESP_OK) {
		LOGD("esp_wifi_connect() rc=%d", errRc);
		duk_error(ctx, 1, "esp_wifi_connect rc=%s", esp32_errToString(errRc));
	}

	LOGD("<< js_wifi_connect");
  return 0;
} // js_wifi_connect


static duk_ret_t js_wifi_disconnect(duk_context *ctx) {
	LOGD(">> js_wifi_disconnect");
	esp_err_t errRc = esp_wifi_disconnect();
	if (errRc != ESP_OK) {
		LOGE("esp_wifi_disconnect: %s", esp32_errToString(errRc));
	}
	LOGD("<< js_wifi_disconnect");
	return 0;
} // js_wifi_disconnect


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
		LOGD("DNS: %d = %s", i, ipString);

		duk_push_string(ctx, ipString);
		// [0] - New array - DNS servers
		// [1] - IP address

		duk_put_prop_index(ctx, idx, i);
		// [0] - New array - DNS servers
	}

	LOGD("<< js_wifi_getDNS");
	return 1; // New array - DNS servers
} // js_wifi_getDNS


/**
 * WIFI.getState()
 * Return an object that describes the state of the WiFi environment.
 * {
 *    isStation:
 *    isAccessPoint:
 *    apMac:
 *    staMac:
 *    country:
 * }
 */
static duk_ret_t js_wifi_getState(duk_context *ctx) {
	wifi_mode_t mode;

	int isAccessPoint;
	int isStation;
	ESP_ERROR_CHECK(esp_wifi_get_mode(&mode));
	switch(mode) {
	case WIFI_MODE_NULL:
		isAccessPoint = 0;
		isStation = 0;
		break;
	case WIFI_MODE_STA:
		isAccessPoint = 0;
		isStation = 1;
		break;
	case WIFI_MODE_AP:
		isAccessPoint = 1;
		isStation = 0;
		break;
	case WIFI_MODE_APSTA:
		isAccessPoint = 1;
		isStation = 1;
		break;
	default:
		isAccessPoint = 0;
		isStation = 0;
		break;
	}


	char apMacString[20];
	char staMacString[20];
	uint8_t mac[6];

	ESP_ERROR_CHECK(esp_wifi_get_mac(WIFI_IF_AP, mac));
	sprintf(apMacString, MACSTR, MAC2STR(mac));

	ESP_ERROR_CHECK(esp_wifi_get_mac(WIFI_IF_STA, mac));
	sprintf(staMacString, MACSTR, MAC2STR(mac));

	wifi_country_t country;
	ESP_ERROR_CHECK(esp_wifi_get_country(&country));
	char *countryString;
	switch(country) {
	case WIFI_COUNTRY_CN:
		countryString = "CN";
		break;
	case WIFI_COUNTRY_EU:
		countryString = "EU";
		break;
	case WIFI_COUNTRY_JP:
		countryString = "JP";
		break;
	case WIFI_COUNTRY_US:
		countryString = "US";
		break;
	default:
		countryString = "Unknown";
		break;
	}

	tcpip_adapter_ip_info_t staIpInfo;
	char staIpString[20];
	tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &staIpInfo);
	sprintf(staIpString, IPSTR, IP2STR(&staIpInfo.ip));

	tcpip_adapter_ip_info_t apIpInfo;
	char apIpString[20];
	tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_AP, &apIpInfo);
	sprintf(apIpString, IPSTR, IP2STR(&apIpInfo.ip));

	duk_push_object(ctx);
	duk_push_boolean(ctx, isStation);
	duk_put_prop_string(ctx, -2, "isStation");
	duk_push_boolean(ctx, isAccessPoint);
	duk_put_prop_string(ctx, -2, "isAccessPoint");
	duk_push_string(ctx, apMacString);
	duk_put_prop_string(ctx, -2, "apMac");
	duk_push_string(ctx, staMacString);
	duk_put_prop_string(ctx, -2, "staMac");
	duk_push_string(ctx, countryString);
	duk_put_prop_string(ctx, -2, "country");
	duk_push_string(ctx, staIpString);
	duk_put_prop_string(ctx, -2, "staIp");
	duk_push_string(ctx, apIpString);
	duk_put_prop_string(ctx, -2, "apIp");

	return 1;
} // End of js_wifi_getState_func


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

  // Check to see if we have a callback function, if we do, stash it.
  if (duk_is_function(ctx, -1)) {
  	// we have a callback
  	g_apStartCallbackStashKey = esp32_duktape_stash_array(ctx, 1);
  } else {
  	duk_pop(ctx); // Remove the thing that might have been a callback function
  }

  // [0] - Options

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

	esp_err_t errRc = esp_wifi_stop();
	if (errRc != ESP_OK) {
		duk_error(ctx, 1, "esp_wifi_stop rc=%s", esp32_errToString(errRc));
	}

	errRc = esp_wifi_set_mode(WIFI_MODE_AP);
	if (errRc != ESP_OK) {
		duk_error(ctx, 1, "esp_wifi_set_mode rc=%s", esp32_errToString(errRc));
	}

	LOGD(" - Being an access point: %s with auth mode %s", ssid, auth);
	wifi_config_t ap_config;
	strcpy((char *)ap_config.ap.ssid, ssid);
	ap_config.ap.ssid_len = 0;
	strcpy((char *)ap_config.ap.password, password);
	ap_config.ap.channel = 0;
	ap_config.ap.authmode = authMode;
	ap_config.ap.ssid_hidden = 0;
	ap_config.ap.max_connection = 4;
	ap_config.ap.beacon_interval = 100;

	errRc = esp_wifi_set_config(WIFI_IF_AP, &ap_config);
	if (errRc != ESP_OK) {
		duk_error(ctx, 1, "esp_wifi_set_config rc=%s", esp32_errToString(errRc));
	}

	errRc = esp_wifi_start();
	if (errRc != ESP_OK) {
		duk_error(ctx, 1, "esp_wifi_start rc=%s", esp32_errToString(errRc));
	}

	/*
	errRc = esp_wifi_connect();
	if (errRc != ESP_OK) {
		duk_error(ctx, 1, "esp_wifi_connect rc=%s", esp32_errToString(errRc));
	}
	*/
	return 0;
} // js_wifi_listen


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


/*
 * [0] - An array of dotted decimal IP addresses.
 */
static duk_ret_t js_wifi_setDNS(duk_context *ctx) {
	LOGD(">> js_wifi_setDNS");
	int length = duk_get_length(ctx, -1);
	if (length > DNS_MAX_SERVERS) {
		length = DNS_MAX_SERVERS;
	}

	int i;
	for (i=0; i<length; i++) {
		duk_get_prop_index(ctx, -1, i);
		// [0] - An array of dotted decimal IP addresses.
		// [1] - An IP address
		const char *ipAddrString = duk_get_string(ctx, -1);
		LOGD("Setting DNS[%d] to %s", i, ipAddrString);
		ip_addr_t ipAddr;
		int rc = ipaddr_aton(ipAddrString, &ipAddr);
		if (rc != 1) {
			LOGE("Failed to parse IP address: %s", ipAddrString);
			return 0;
		}
		dns_setserver(i, &ipAddr);
		duk_pop(ctx);
	}
	LOGD("<< js_wifi_setDNS");
	return 0;
}


static duk_ret_t js_wifi_start(duk_context *ctx) {
	LOGD(">> js_wifi_start");
	esp_err_t errRc = esp_wifi_start();
	if (errRc != ESP_OK) {
		duk_error(ctx, 1, "js_wifi_start rc=%s", esp32_errToString(errRc));
	}
	LOGD("<< js_wifi_start");
	return 0;
} // js_wifi_start


static duk_ret_t js_wifi_stop(duk_context *ctx) {
	LOGD(">> js_wifi_stop");
	esp_err_t errRc = esp_wifi_stop();
	if (errRc != ESP_OK) {
		duk_error(ctx, 1, "js_wifi_stop rc=%s", esp32_errToString(errRc));
	}
	LOGD("<< js_wifi_stop");
	return 0;
} // js_wifi_stop


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
 * Create the WIFI module in Global.
 */
void ModuleWIFI(duk_context *ctx) {
	duk_push_global_object(ctx);
	// [0] - Global object

	duk_idx_t idx = duk_push_object(ctx); // Create new WIFI object
	// [0] - Global object
	// [1] - New object

	duk_push_c_function(ctx, js_wifi_connect, 2);
	// [0] - Global object
	// [1] - New object
	// [2] - c-func - js_wifi_connect

	duk_put_prop_string(ctx, idx, "connect"); // Add connect to new WIFI
	// [0] - Global object
	// [1] - New object

	duk_push_c_function(ctx, js_wifi_disconnect, 0);
	// [0] - Global object
	// [1] - New object
	// [2] - c-func - js_wifi_disconnect

	duk_put_prop_string(ctx, idx, "disconnect"); // Add connect to new WIFI
	// [0] - Global object
	// [1] - New object

	duk_push_c_function(ctx, js_wifi_getDNS, 0);
	// [0] - Global object
	// [1] - New object
	// [2] - c-func - js_wifi_getDNS

	duk_put_prop_string(ctx, idx, "getDNS"); // Add getDNS to new WIFI
	// [0] - Global object
	// [1] - New object

	duk_push_c_function(ctx, js_wifi_getState, 0);
	// [0] - Global object
	// [1] - New object
	// [2] - c-func - js_wifi_getState

	duk_put_prop_string(ctx, idx, "getState"); // Add getState to new WIFI
	// [0] - Global object
	// [1] - New object

	duk_push_c_function(ctx, js_wifi_listen, 2);
	// [0] - Global object
	// [1] - New object
	// [2] - c-func - js_wifi_listen

	duk_put_prop_string(ctx, idx, "listen"); // Add listen to new WIFI
	// [0] - Global object
	// [1] - New object

	duk_push_c_function(ctx, js_wifi_scan, 1);
	// [0] - Global object
	// [1] - New object
	// [2] - c-func - js_wifi_scan

	duk_put_prop_string(ctx, idx, "scan"); // Add scan to new WIFI
	// [0] - Global object
	// [1] - New object

	duk_push_c_function(ctx, js_wifi_setDNS, 1);
	// [0] - Global object
	// [1] - New object
	// [2] - c-func - js_wifi_setDNS

	duk_put_prop_string(ctx, idx, "setDNS"); // Add setDNS to new WIFI
	// [0] - Global object
	// [1] - New object

	duk_push_c_function(ctx, js_wifi_start, 0);
	// [0] - Global object
	// [1] - New object
	// [2] - c-func - js_wifi_start

	duk_put_prop_string(ctx, idx, "start"); // Add start to new WIFI
	// [0] - Global object
	// [1] - New object


	duk_push_c_function(ctx, js_wifi_stop, 0);
	// [0] - Global object
	// [1] - New object
	// [2] - c-func - js_wifi_stop

	duk_put_prop_string(ctx, idx, "stop"); // Add stop to new WIFI
	// [0] - Global object
	// [1] - New object


	duk_put_prop_string(ctx, 0, "WIFI"); // Add WIFI to global
	// [0] - Global object

	duk_pop(ctx);

	// Setup the new Event handler
	esp_event_loop_set_cb(esp32_wifi_eventHandler, NULL); // No return code to check.
} // ModuleWIFI
