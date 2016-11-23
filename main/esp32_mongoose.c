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
static struct mg_connection *g_nc;
static struct mg_mgr mgr;
static uint32_t g_counter = 0;
static struct mg_connection *g_webSocketConnection;

/**
 * Convert a Mongoose event type to a string.
 */
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
	case MG_EV_WEBSOCKET_HANDSHAKE_REQUEST:
		return "MG_EV_WEBSOCKET_HANDSHAKE_REQUEST";
	case MG_EV_WEBSOCKET_HANDSHAKE_DONE:
		return "MG_EV_WEBSOCKET_HANDSHAKE_DONE";
	case MG_EV_WEBSOCKET_FRAME:
		return "MG_EV_WEBSOCKET_FRAME";
	}
	sprintf(temp, "Unknown event: %d", ev);
	return temp;
} //eventToString


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
	ESP_LOGD(tag, ">> mongoose_event_handler: [task=%s] %d %s",
			pcTaskGetTaskName(NULL),
			(uint32_t)nc->user_data,
			mongoose_eventToString(ev));
	switch (ev) {
		case MG_EV_ACCEPT: {
			nc->user_data = (void *)g_counter;
			g_counter++;
			break;
		}
		case MG_EV_WEBSOCKET_HANDSHAKE_DONE: {
			g_webSocketConnection = nc;
			break;
		}
		case MG_EV_HTTP_REQUEST: {
			g_nc = nc;

			struct http_message *message = (struct http_message *) evData;
			char *uri = mgStrToStr(message->uri);
			char *method = mgStrToStr(message->method);
			ESP_LOGD(tag, "- %s %s", method, uri);
			if (strcmp(method, "OPTIONS") == 0) {
				// Handle OPTIONS
				mg_send_head(nc, 200, 0,
						"Access-Control-Allow-Origin: *\n"
						"Access-Control-Allow-Headers: Content-Type\n"
						"Content-Type: text/plain");
				nc->flags |= MG_F_SEND_AND_CLOSE;
				free(uri);
				free(method);
			}
			// If the command is POST /run then this is a request to execute the
			// JavaScript supplied in the post body.
			else if (strcmp(method, "POST") == 0 && strcmp(uri, "/run") ==0) {
				// A Run request
				char *body = mgStrToStr(message->body);
				newCommandLineEvent(body, strlen(body), 0 /* NOT from keyboard */);
				free(body);
				free(uri);
				free(method);
				mg_send_head(nc, 200, 0, "Access-Control-Allow-Origin: *");
				nc->flags |= MG_F_SEND_AND_CLOSE;
			}	else {
				newHTTPServerRequestEvent(uri, method);
			}
			break;
		} // End of MG_EV_HTTP_REQUEST
	} // End of switch
	ESP_LOGD(tag, "<< mongoose_event_handler");
} // End of mongoose_event_handler


/**
 * This is the FreeRTOS task that initializes and runs mongoose.  It should
 * only ever be run once.
 */
static void mongooseTask(void *data) {
	ESP_LOGD(tag, "Mongoose task starting");

	char address[20];
	sprintf(address, ":%d", g_port);

	ESP_LOGD(tag, "Mongoose: Starting setup");
	mg_mgr_init(&mgr, NULL);

	ESP_LOGD(tag, "Mongoose: Succesfully inited");
	struct mg_connection *conn = mg_bind(&mgr, address, mongoose_event_handler);

	ESP_LOGD(tag, "Mongoose: Successfully bound to address \"%s\"", address);
	if (conn == NULL) {
		ESP_LOGE(tag, "Mongoose: No connection from the mg_bind()");
		vTaskDelete(NULL);
		return;
	}

	mg_set_protocol_http_websocket(conn);

	// Perform the polling loop forever ...
	while (1) {
		mg_mgr_poll(&mgr, 200);
	} // End while

} // mongooseTask


void startMongooseServer() {
	if (g_mongooseStarted) { // Check to see if we are ALREADY running a mongoose task.
		return ;
	}
	g_mongooseStarted = 1; // Flag that we have now started a mongoose task so we don't start a second.
	xTaskCreatePinnedToCore(&mongooseTask, "mongoose_task", 4000, NULL, 5, NULL,0);
} // startMongooseServer

/**
 * Start mongoose.
 * [ 0] - function - requestReceivedCallback.
 */
duk_ret_t js_startMongoose(duk_context *ctx) {
	startMongooseServer();
	return 0;
} // startMongoose


typedef struct {
	int status; // The status code to be returned with the server response.
	char *body; // The body of the response as a NULL terminated string. malloc() storage.
} server_response_t;


/**
 * When working with mongoose, we are NOT allowed to call mongoose functions
 * on separate threads.  Instead, we create event handler functions that are run
 * within the context of the mongoose environment.  This function is an example
 * of such a handler which is responsible for sending HTTP server response.
 */
static void server_response_event_handler(struct mg_connection *nc, int ev, void *evData) {
	ESP_LOGD(tag, ">> server_response_event_handler");
	server_response_t *pServerResponse = (server_response_t *)evData;
	size_t length = strlen(pServerResponse->body);
	ESP_LOGD(tag, "- status: %d, length_of_data: %d", pServerResponse->status, length);
	mg_send_head(g_nc, pServerResponse->status, length, NULL);
	mg_send(g_nc, pServerResponse->body, length);
	g_nc->flags |= MG_F_SEND_AND_CLOSE;
	free(pServerResponse->body);
	ESP_LOGD(tag, "<< server_response_event_handler");
} // server_response_event_handler


/**
 * Called to send a response back to the waiting browser client.  We do this
 * by populating a structure which then is passed to an event handler so that
 * mongoose can process the request on its single thread.
 * [ 0] - number - status
 * [ 1] - object - headers
 * [ 2] - string - body
 */
duk_ret_t js_serverResponseMongoose(duk_context *ctx) {
	ESP_LOGD(tag, ">> serverResponseMongoose: [task=%s]", pcTaskGetTaskName(NULL));

	//esp32_duktape_dump_value_stack(ctx); // Debug
	int status = duk_get_int(ctx, 0); // Get the status from the Duktape value stack.
	const char *body = duk_get_string(ctx, 2); // Get the body from the Duktape value stack.
	ESP_LOGD(tag, "- status: %d, length_of_data: %d", status, strlen(body));

	server_response_t serverResponse;
	serverResponse.status = status;
	serverResponse.body = strdup(body);
	ESP_LOGD(tag, "- Broadcasting event");
	mg_broadcast(&mgr, server_response_event_handler, &serverResponse, sizeof(serverResponse));

	ESP_LOGD(tag, "<< serverResponseMongoose");
	return 0;
} // serverResponseMongoose


/**
 * Send messages to any websocket attached consoles.
 */
void websocket_console_sendData(const char *message) {
	if (g_webSocketConnection == NULL) {
		return;
	}
	mg_send_websocket_frame(
	g_webSocketConnection, WEBSOCKET_OP_TEXT, message, strlen(message));
} // websocket_console_sendData
