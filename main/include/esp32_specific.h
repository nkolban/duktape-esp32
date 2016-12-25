/*
 * esp32_specific.h
 *
 *  Created on: Nov 18, 2016
 *      Author: kolban
 */

#if !defined(MAIN_ESP32_SPECIFIC_H_)
#define MAIN_ESP32_SPECIFIC_H_

char *esp32_errToString(esp_err_t value);
char *esp32_loadFileESPFS(const char *path, size_t *fileSize);
void setupVFS();
void setupWebVFS(const char *mountPoint, char *baseURL);

#endif /* MAIN_ESP32_SPECIFIC_H_ */
