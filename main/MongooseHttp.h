/*
 * MongooseHttp.h
 *
 *  Created on: Nov 30, 2016
 *      Author: kolban
 */

#ifndef MAIN_MONGOOSEHTTP_H_
#define MAIN_MONGOOSEHTTP_H_
#include <string>
#include <stdint.h>

class MongooseHttp {
public:
	MongooseHttp();
	virtual ~MongooseHttp();
	void setProtocol(char *protocol);
	void setHostname(char *hostname);
	void setMethod(char *method);
	void setPath(char *path);
	void setPort(uint16_t port);
	void send();
private:
	std::string protocol = "http";
	std::string hostname = "localhost";
	uint16_t port = 80;
	std::string method="GET";
	std::string path="/";

};

#endif /* MAIN_MONGOOSEHTTP_H_ */
