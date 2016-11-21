/**
 *
 * Neil Kolban <kolban1@kolban.com>
 *
 */
#include <freertos/FreeRTOS.h>
#include <esp_log.h>
#include <mongoose.h>
#include <duktape.h>
#include "esp32_duktape/duktape_event.h"
#include "duktape_utils.h"
#include "sdkconfig.h"

static char tag []="esp32_mongoose";
static unsigned short g_port=80;
static uint8_t g_mongooseStarted = 0;
struct mg_connection *g_nc;
/**
 * Convert a Mongoose event type to a string.
 */
/*
static char *mongoose_eventToString(int ev) {
	static char temp[100];
	switch (ev) {
	case MG_EV_CONNECT:
		return "MG_EV_CONNECT";
	case MG_EV_ACCEPT:
		return "MG_EV_ACCEPT";
	case MG_EV_CLOSE:
		return "MG_EV_CLOSE";
	case MG_EV_SEND:
		return "MG_EV_SEND";
	case MG_EV_RECV:
		return "MG_EV_RECV";
	case MG_EV_HTTP_REQUEST:
		return "MG_EV_HTTP_REQUEST";
	case MG_EV_MQTT_CONNACK:
		return "MG_EV_MQTT_CONNACK";
	case MG_EV_MQTT_CONNACK_ACCEPTED:
		return "MG_EV_MQTT_CONNACK";
	case MG_EV_MQTT_CONNECT:
		return "MG_EV_MQTT_CONNECT";
	case MG_EV_MQTT_DISCONNECT:
		return "MG_EV_MQTT_DISCONNECT";
	case MG_EV_MQTT_PINGREQ:
		return "MG_EV_MQTT_PINGREQ";
	case MG_EV_MQTT_PINGRESP:
		return "MG_EV_MQTT_PINGRESP";
	case MG_EV_MQTT_PUBACK:
		return "MG_EV_MQTT_PUBACK";
	case MG_EV_MQTT_PUBCOMP:
		return "MG_EV_MQTT_PUBCOMP";
	case MG_EV_MQTT_PUBLISH:
		return "MG_EV_MQTT_PUBLISH";
	case MG_EV_MQTT_PUBREC:
		return "MG_EV_MQTT_PUBREC";
	case MG_EV_MQTT_PUBREL:
		return "MG_EV_MQTT_PUBREL";
	case MG_EV_MQTT_SUBACK:
		return "MG_EV_MQTT_SUBACK";
	case MG_EV_MQTT_SUBSCRIBE:
		return "MG_EV_MQTT_SUBSCRIBE";
	case MG_EV_MQTT_UNSUBACK:
		return "MG_EV_MQTT_UNSUBACK";
	case MG_EV_MQTT_UNSUBSCRIBE:
		return "MG_EV_MQTT_UNSUBSCRIBE";
	}
	sprintf(temp, "Unknown event: %d", ev);
	return temp;
} //eventToString
*/

// Convert a Mongoose string type to a string.
static char *mgStrToStr(struct mg_str mgStr) {
	char *retStr = (char *) malloc(mgStr.len + 1);
	memcpy(retStr, mgStr.p, mgStr.len);
	retStr[mgStr.len] = 0;
	return retStr;
} // mgStrToStr


/**
 * Mongoose event handler.
 * The evData contains the data structures for specific events.
 * For MG_EV_HTTP_REQUEST - struct http_message. (https://docs.cesanta.com/mongoose/master/#/c-api/http.h/struct_http_message.md/)
 *
 */
static void mongoose_event_handler(struct mg_connection *nc, int ev, void *evData) {
	switch (ev) {
		case MG_EV_HTTP_REQUEST: {
			ESP_LOGD(tag, ">> mongoose_event_handler: MG_EV_HTTP_REQUEST");
			esp32_duktape_event_t event;
			g_nc = nc;

			struct http_message *message = (struct http_message *) evData;
			char *uri = mgStrToStr(message->uri);
			char *method = mgStrToStr(message->method);
			newHTTPServerRequestEvent(&event, uri, method);
			ESP_LOGD(tag, "<< mongoose_event_handler");
			break;
		} // End of MG_EV_HTTP_REQUEST
	} // End of switch

} // End of mongoose_event_handler


// FreeRTOS task to start Mongoose.
static void mongooseTask(void *data) {
	char address[20];
	sprintf(address, ":%d", g_port);
	ESP_LOGD(tag, "Mongoose task starting");
	struct mg_mgr mgr;
	ESP_LOGD(tag, "Mongoose: Starting setup");
	mg_mgr_init(&mgr, NULL);
	ESP_LOGD(tag, "Mongoose: Succesfully inited");
	struct mg_connection *c = mg_bind(&mgr, address, mongoose_event_handler);
	ESP_LOGD(tag, "Mongoose Successfully bound to address \"%s\"", address);
	if (c == NULL) {
		ESP_LOGE(tag, "No connection from the mg_bind()");
		vTaskDelete(NULL);
		return;
	}
	mg_set_protocol_http_websocket(c);

	while (1) {
		mg_mgr_poll(&mgr, 200);
	}
} // mongooseTask

/**
 * Start mongoose.
 * [ 0] - function - requestReceivedCallback.
 */
duk_ret_t startMongoose(duk_context *ctx) {
	if (g_mongooseStarted) {
		return 0;
	}
	g_mongooseStarted = 1;
	esp32_duktape_dump_value_stack(ctx);
	xTaskCreatePinnedToCore(&mongooseTask, "mongooseTask", 4000, NULL, 5, NULL,0);
	return 0;
}

/**
 * Called to send a response back to the waiting browser client.
 * [ 0] - number - status
 * [ 1] - object - headers
 * [ 2] - string - body
 */
duk_ret_t serverResponseMongoose(duk_context *ctx) {
	ESP_LOGD(tag, ">> serverResponseMongoose");
	//esp32_duktape_dump_value_stack(ctx); // Debug
	int status = duk_get_int(ctx, 0);
	//void *headers = NULL;
	const char *body = duk_get_string(ctx, 2);
	size_t length = strlen(body);
	ESP_LOGD(tag, "- status: %d, length_of_data: %d", status, length);

	mg_send_head(g_nc, status, length, NULL);
	mg_send(g_nc, body, length);
	g_nc->flags |= MG_F_SEND_AND_CLOSE;

	ESP_LOGD(tag, "<< serverResponseMongoose");
	return 0;
} // serverResponseMongoose
