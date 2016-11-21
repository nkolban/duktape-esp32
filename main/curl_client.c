/*
 * curl_client.c
 *
 *  Created on: Nov 13, 2016
 *      Author: kolban
 *
 * Web:
 * libcurl - https://curl.haxx.se/libcurl/
 * CURL examples - https://curl.haxx.se/libcurl/c/example.html
 */

#include <esp_log.h>
#include <curl/curl.h>
#include <string.h>
#include <stddef.h>
#include <assert.h>
#include <freertos/event_groups.h>
#include "esp32_duktape/curl_client.h"
#include "esp32_duktape/duktape_event.h"
#include "sdkconfig.h"

static char tag[] = "curl_client";

typedef struct {
	size_t size; // The size of the data
	uint8_t *data; // The data received
} context_t;
/**
 * Receive data from a CURL callback.
 */
static size_t writeData(void *buffer, size_t size, size_t nmemb, void *userp) {
	ESP_LOGD(tag, "writeData: size=%d, number=%d", size, nmemb);
	ESP_LOGD(tag, "data: %.*s",size*nmemb, (char *)buffer);
	size_t total = size * nmemb;
	context_t *context = (context_t *)userp;
	if (context->size == 0) {
		context->data = malloc(total);
	} else {
		context->data = realloc(context->data, context->size + total);
	}
	memcpy(context->data + context->size, buffer, total);
	context->size += total;
	return total;
}

/**
 * Get the content from a CURL client request to a remote
 * URL.  The return is a NULL terminated string that contains
 * that content.  If an error occurred or the file was not found
 * on the server, then NULL is returned.
 *
 * The return data is malloced and needs to be freed when done.
 */
char *curl_client_getContent(const char *url) {

	CURL *curlHandle;
	CURLcode res;
	char *retData = NULL;

	ESP_LOGD(tag, ">> curl_client: %s", url==NULL?"NULL":url);
	if (url == NULL) {
		return NULL;
	}

	curlHandle = curl_easy_init();
	if(curlHandle) {
		context_t *context = (context_t *)malloc(sizeof(context_t));
		assert(context != NULL);
		context->size = 0;
		context->data = NULL;

		curl_easy_setopt(curlHandle, CURLOPT_URL, url);
		curl_easy_setopt(curlHandle, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(curlHandle, CURLOPT_WRITEFUNCTION, writeData); // Write data callback.
		curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, context); // User data to pass to callback.
		curl_easy_setopt(curlHandle, CURLOPT_FAILONERROR, 1);

		curl_easy_setopt(curlHandle, CURLOPT_VERBOSE, 1);		// Enable debugging

		/* Perform the request, res will get the return code */
		res = curl_easy_perform(curlHandle);
		ESP_LOGD(tag, "curl_easy_perform: rc=%d, size=%d", res, context->size);
		/* Check for errors */
		if(res != CURLE_OK) {
			ESP_LOGE(tag, "curl_easy_perform() failed: %s", curl_easy_strerror(res));
		}

		/* always cleanup */
		curl_easy_cleanup(curlHandle);
		curl_global_cleanup();

		if (res == CURLE_OK && context->size > 0) {
			retData = malloc(context->size + 1);
			memcpy(retData, context->data, context->size);
			retData[context->size] = 0;
		}

		if (context->data != NULL) {
			free(context->data);
		}

		free(context);
	}
	ESP_LOGD(tag, "<< curl_client");
	return retData;
} // End of curl_client_getContent


/**
 * Make an HTTP request using the pOptions as data.
 *
 * char *protocol;
 * char *host;
 * uint16_t port;
 * char *method;
 * char *path;
 */
void curl_client_http_request(struct http_request_options *pOptions) {
	CURL *curlHandle;
	CURLcode res;
	char *retData = NULL;

	char url[256];
	ESP_LOGD(tag, ">> curl_client_http_request");
	sprintf(url, "%s://%s:%d%s", pOptions->protocol, pOptions->host, pOptions->port, pOptions->path);
	ESP_LOGD(tag, " url: %s", url);
	curlHandle = curl_easy_init();
	if(curlHandle) {
		context_t *context = (context_t *)malloc(sizeof(context_t));
		context->size = 0;
		curl_easy_setopt(curlHandle, CURLOPT_URL, url);
		/* example.com is redirected, so we tell libcurl to follow redirection */
		curl_easy_setopt(curlHandle, CURLOPT_FOLLOWLOCATION, 1L);
		// Set the callback function
		curl_easy_setopt(curlHandle, CURLOPT_WRITEFUNCTION, writeData);
		curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, context);
		curl_easy_setopt(curlHandle, CURLOPT_FAILONERROR, 1);

		/* Perform the request, res will get the return code */
		res = curl_easy_perform(curlHandle);
		ESP_LOGD(tag, "curl_easy_perform: rc=%d, size=%d", res, context->size);
		/* Check for errors */
		if(res != CURLE_OK) {
			ESP_LOGE(tag, "curl_easy_perform() failed: %s", curl_easy_strerror(res));
		}

		/* always cleanup */
		curl_easy_cleanup(curlHandle);
		if (res == CURLE_OK && context->size > 0) {
			retData = malloc(context->size + 1);
			memcpy(retData, context->data, context->size);
			retData[context->size] = 0;
		}
		if (context->data != NULL) {
			free(context->data);
		}
		free(context);
	}
	ESP_LOGD(tag, "<< curl_client_http_request");
} // End of curl_client_http_request
