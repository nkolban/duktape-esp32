/**
 * Within our story we want the ability to call back into JS from a C environment.
 * This is typically the case when an asynchronous event occurs such as a timer firing
 * or a web response being made available.
 *
 * By our rules, we may not arbitrarily call back into the JS runtime when it has control.
 * Instead what we will do is post an event and when otherwise idle, the event will be
 * picked off the event queue and processed.  One of the types of events we can post
 * is a CallbackRequested.  This is a general purpose event that says "Hey, JS, here
 * is a callback I want you to invoke and with these parameters".
 *
 * The event processor will invoke:
 *
 * callback_perform(type, context, data)
 *
 * Where "type" is the type of callback to perform.
 * "context" is the context of the callback.
 * "data" is specific data to be passed into the callback.
 *
 * The implementation of callback_perform() knows how to process each
 * distinct type of callback.
 *
 *
 * function eventCallback(callbackType, contextData, appData) {
    console.log("eventCallback called! \nContextData:" + contextData + "\nappData: " + appData);
}
 */
#include "duktape_callback.h"
static void process_http_response(void *context, void *data) {

}

void callback_perform(int type, void *context, void *data) {
	if (type == CALLBACK_HTTP_RESPONSE) {
		process_http_response(context, data);
	}
}
