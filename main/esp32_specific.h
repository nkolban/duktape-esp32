/*
 * esp32_specific.h
 *
 *  Created on: Nov 18, 2016
 *      Author: kolban
 */

#ifndef MAIN_ESP32_SPECIFIC_H_
#define MAIN_ESP32_SPECIFIC_H_

void setupVFS();
void setupWebVFS(const char *mountPoint, char *baseURL);

#endif /* MAIN_ESP32_SPECIFIC_H_ */
