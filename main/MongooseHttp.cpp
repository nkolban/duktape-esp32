/*
 * MongooseHttp.cpp
 *
 *  Created on: Nov 30, 2016
 *      Author: kolban
 */

#include "MongooseHttp.h"
#include "esp32_mongoose.h"
#include <iostream>
#include <string>
#include <esp_log.h>

static char tag[] = "MongooseHttp";

MongooseHttp::MongooseHttp() {
	// TODO Auto-generated constructor stub

}


MongooseHttp::~MongooseHttp() {
	// TODO Auto-generated destructor stub
}


void MongooseHttp::setProtocol(char* protocol) {
	this->protocol = protocol;
} // setProtocol


void MongooseHttp::setHostname(char* hostname) {
	this->hostname = hostname;
} // setHostname


void MongooseHttp::setMethod(char* method) {
	this->method = method;
} // setMethod


void MongooseHttp::setPath(char* path) {
	this->path = path;
} // setPath


void MongooseHttp::setPort(uint16_t port) {
	this->port = port;
} // setPort

void MongooseHttp::send() {
	// http://www.google.com:80/hello
	char portBuf[10];
	itoa(port, portBuf, 10);
	std::string url = protocol + "://" + hostname + ":" + portBuf + path;
	ESP_LOGD(tag, "Target URL: %s", url.c_str());
	esp32_mongoose_sendHTTPRequest(url.c_str());
} // send
