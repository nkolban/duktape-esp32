/**
 * ESP32 specific functions supporting the Duktape project.
 */

#include <esp_log.h>
#include <esp_vfs.h>
#include <esp_wifi.h>
#include <espfs.h>
#include <fcntl.h>
#include <math.h>
#include <nvs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "logging.h"
#include "sdkconfig.h"

LOG_TAG("esp32_specific");

/*
static char *g_baseURL;

#define MAX_WEB_FILE_RECORDS (10)
typedef struct {
	int inUse;
	int fd; // File descriptor
	uint8_t *data; // Data
	ssize_t size; // Size
	ssize_t fptr; // File Pointer
} file_record_t;

static file_record_t file_records[MAX_WEB_FILE_RECORDS];
static file_record_t *getNextFreeFilerecord();
static file_record_t *getFilerecord(int fd);
*/


/*
 * Load a file using the ESPFS file system.  The file to be loaded is specified
 * by the "path" parameter.  If the file can be loaded, then the full data
 * of the file is returned and the "fileSize" size pointer is updated.
 * If the file can't be loaded, NULL is returned.
 *
 * If data is returned, it does NOT need to be cleaned up because the nature of
 * the ESPFS file system is that the data is found in flash and addressable.  As
 * such there is no RAM cost for getting the data of such a file.
 *
 * Prior to calling this function, the espFsInit() function should have been
 * previously called to initialize the ESPFS environment.
 */
char *esp32_loadFileESPFS(char *path, size_t *fileSize) {
	EspFsFile *fh = espFsOpen((char *)path);
	if (fh == NULL) {
		*fileSize = 0;
		LOGD("ESPFS: Failed to open file %s", path);
		return NULL;
	}
  char *data;
  espFsAccess(fh, (void **)&data, fileSize);
  espFsClose(fh);
  // Note ... because data is mapped in memory from flash ... it will be good
  // past the file close.
  LOGD("esp32_loadFileESPFS: Read file %s for size %d", path, *fileSize);
  return data;
} // esp32_loadFileESPFS


/**
 * Log the flags that are specified in an open() call.
 */
/*
static void logOpenFlags(int flags) {
	LOGD("flags:");
	if (flags & O_APPEND) {
		LOGD("- O_APPEND");
	}
	if (flags & O_CREAT) {
		LOGD("- O_CREAT");
	}
	if (flags & O_TRUNC) {
		LOGD("- O_TRUNC");
	}
	if (flags & O_RDONLY) {
		LOGD("- O_RDONLY");
	}
	if (flags & O_WRONLY) {
		LOGD("- O_WRONLY");
	}
	if (flags & O_RDWR) {
		LOGD("- O_RDWR");
	}
} // End of logFlags


static size_t vfs_write(int fd, const void *data, size_t size) {
	LOGD(">> write fd=%d, data=0x%lx, size=%d", fd, (unsigned long)data, size);
	return 0;
}


static off_t vfs_lseek(int fd, off_t offset, int whence) {
	LOGD(">> lseek fd=%d, offset=%d, whence=%d", fd, (int)offset, whence);
	return 0;
}


static ssize_t vfs_read(int fd, void *dst, size_t size) {
	LOGD(">> read fd=%d, dst=0x%lx, size=%d", fd, (unsigned long)dst, size);
	return 0;
}
*/

/**
 * Open the file specified by path.  The flags contain the instructions
 * on how the file is to be opened.  For example:
 *
 * O_CREAT  - Create the named file.
 * O_TRUNC  - Truncate (empty) the file.
 * O_RDONLY - Open the file for reading only.
 * O_WRONLY - Open the file for writing only.
 * O_RDWR   - Open the file for reading and writing.
 *
 * The mode are access mode flags.
 */
/*
static int vfs_open(const char *path, int flags, int accessMode) {
	LOGD(">> open path=%s, flags=0x%x, accessMode=0x%x", path, flags, accessMode);
	logOpenFlags(flags);
	return -1;
}


static int vfs_close(int fd) {
	LOGD(">> close fd=%d", fd);
	return 0;
}


static int vfs_fstat(int fd, struct stat *st) {
	LOGD(">> fstat fd=%d", fd);
	return 0;
}


static int vfs_stat(const char *path, struct stat *st) {
	LOGD(">> stat path=%s", path);
	return 0;
}


static int vfs_link(const char *oldPath, const char *newPath) {
	LOGD(">> link oldPath=%s, newPath=%s", oldPath, newPath);
	return 0;
}


static int vfs_rename(const char *oldPath, const char *newPath) {
	LOGD(">> rename oldPath=%s, newPath=%s", oldPath, newPath);
	return 0;
}
*/

/**
 * Register the VFS at the specified mount point.
 * The callback functions are registered to handle the
 * different functions that may be requested against the
 * VFS.
 */
/*
void registerTestVFS(char *mountPoint) {

} // End of registerTestVFS
*/
/*
void setupVFS() {
	LOGD(">> setupVFS");
	char *mountPoint = "/modules";
	esp_vfs_t vfs;
	esp_err_t err;

	vfs.fd_offset = 0;
	vfs.flags  = ESP_VFS_FLAG_DEFAULT;
	vfs.write  = vfs_write;
	vfs.lseek  = vfs_lseek;
	vfs.read   = vfs_read;
	vfs.open   = vfs_open;
	vfs.close  = vfs_close;
	vfs.fstat  = vfs_fstat;
	vfs.stat   = vfs_stat;
	vfs.link   = vfs_link;
	vfs.rename = vfs_rename;

	err = esp_vfs_register(mountPoint, &vfs, NULL);
	if (err != ESP_OK) {
		LOGE("setupVFS: esp_vfs_register: err=%d", err);
	}
	LOGD("<< setupVFS");
} // setupVFS

*/



/**
 * Open a file that is actually a web page.  Here we retrieve the content
 * of a web page against an HTTP server where the server is supplied by
 * the path.
 */
/*
static int web_vfs_open(const char *path, int flags, int accessMode) {
	return 0;
} // web_vfs_open


static int web_vfs_close(int fd) {
	LOGD(">> web_vfs_close fd=%d", fd);
	if (fd == -1) {
		return 0;
	}
	file_record_t *pFileRecord = getFilerecord(fd);
	if (pFileRecord == NULL) {
		LOGD("File record for fd=%d not found", fd);
		return -1;
	}
	if (pFileRecord->inUse == 0) {
		LOGD("Unexpected state ... closing fd=%d but it appear to be free", fd);
		return -1;
	}
	free(pFileRecord->data);
	pFileRecord->inUse = 0;
	return 0;
} // web_vfs_close
*/

/**
 * Read the web context into the buffer passed in.  The web content structure
 * we already have contains:
 * * data - The pointer to the web retrieved data.
 * * size - The size of the data retrieved.
 * * fptr - The file pointer of where we currently are within the retrieved data.
 */
/*
static ssize_t web_vfs_read(
		int fd, // The file descriptor to read the data from.
		void *dst, // The buffer where we will write the read data.
		size_t bufferSize // The number of bytes available in the buffer.
	) {
	ssize_t retSize;
	file_record_t *pFileRecord = &file_records[0];
	LOGD(">> web_vfs_read fd=%d, dst=0x%lx, bufferSize=%d", fd, (unsigned long)dst, bufferSize);
	// We must check that the amount of file data requested to copy will fit in the
	// buffer.  If we have been passed in a buffer that is less than the size of
	// the file we read, then we need to adjust.
	if (pFileRecord->size - pFileRecord->fptr > bufferSize) {
		retSize = bufferSize;
	} else {
		retSize = pFileRecord->size;
	}
	memcpy(dst, pFileRecord->data + pFileRecord->fptr, retSize);
	pFileRecord->fptr += retSize;

	LOGD("<< web_vfs_read  retSize = %d", retSize);
	return retSize;
} // web_vfs_read


static file_record_t *getNextFreeFilerecord() {
	int i;
	for (i=0; i<MAX_WEB_FILE_RECORDS; i++) {
		if (!file_records[i].inUse ) {
			return &file_records[i];
		}
	}
	return NULL;
} // getFilerecord

static file_record_t *getFilerecord(int fd) {
	if (fd < 0 || fd >= MAX_WEB_FILE_RECORDS) {
		return NULL;
	}
	return &file_records[fd];
}

void setupWebVFS(const char *mountPoint, char *baseURL) {
	esp_vfs_t vfs;
	esp_err_t err;

	g_baseURL = baseURL;

	vfs.fd_offset = 0;
	vfs.flags  = ESP_VFS_FLAG_DEFAULT;
	vfs.write  = NULL;
	vfs.lseek  = NULL;
	vfs.read   = web_vfs_read;
	vfs.open   = web_vfs_open;
	vfs.close  = web_vfs_close;
	vfs.fstat  = NULL;
	vfs.stat   = NULL;
	vfs.link   = NULL;
	vfs.rename = NULL;

	err = esp_vfs_register(mountPoint, &vfs, NULL);
	if (err != ESP_OK) {
		LOGE("createWebVFS: esp_vfs_register: err=%d", err);
	}
	int i;
	for (i=0; i<MAX_WEB_FILE_RECORDS; i++) {
		file_records[i].inUse = 0;
		file_records[i].fd = i;
	}
} // createWebVFS
*/

/*
 *  Return a string representation of an esp_err_t value.
 */
char *esp32_errToString(esp_err_t value) {
	switch(value) {
	case ESP_OK:
		return "OK";
	case ESP_FAIL:
		return "Fail";
	case ESP_ERR_NO_MEM:
		return "No memory";
	case ESP_ERR_INVALID_ARG:
		return "Invalid argument";
	case ESP_ERR_INVALID_SIZE:
		return "Invalid state";
	case ESP_ERR_INVALID_STATE:
		return "Invalid state";
	case ESP_ERR_NOT_FOUND:
		return "Not found";
	case ESP_ERR_NOT_SUPPORTED:
		return "Not supported";
	case ESP_ERR_TIMEOUT:
		return "Timeout";
	case ESP_ERR_NVS_NOT_INITIALIZED:
		return "ESP_ERR_NVS_NOT_INITIALIZED";
	case ESP_ERR_NVS_NOT_FOUND:
		return "ESP_ERR_NVS_NOT_FOUND";
	case ESP_ERR_NVS_TYPE_MISMATCH:
		return "ESP_ERR_NVS_TYPE_MISMATCH";
	case ESP_ERR_NVS_READ_ONLY:
		return "ESP_ERR_NVS_READ_ONLY";
	case ESP_ERR_NVS_NOT_ENOUGH_SPACE:
		return "ESP_ERR_NVS_NOT_ENOUGH_SPACE";
	case ESP_ERR_NVS_INVALID_NAME:
		return "ESP_ERR_NVS_INVALID_NAME";
	case ESP_ERR_NVS_INVALID_HANDLE:
		return "ESP_ERR_NVS_INVALID_HANDLE";
	case ESP_ERR_NVS_REMOVE_FAILED:
		return "ESP_ERR_NVS_REMOVE_FAILED";
	case ESP_ERR_NVS_KEY_TOO_LONG:
		return "ESP_ERR_NVS_KEY_TOO_LONG";
	case ESP_ERR_NVS_PAGE_FULL:
		return "ESP_ERR_NVS_PAGE_FULL";
	case ESP_ERR_NVS_INVALID_STATE:
		return "ESP_ERR_NVS_INVALID_STATE";
	case ESP_ERR_NVS_INVALID_LENGTH:
		return "ESP_ERR_NVS_INVALID_LENGTH";
	case ESP_ERR_WIFI_NOT_INIT:
		return "ESP_ERR_WIFI_NOT_INIT";
	case ESP_ERR_WIFI_NOT_STARTED:
		return "ESP_ERR_WIFI_NOT_STARTED";
	case ESP_ERR_WIFI_IF:
		return "ESP_ERR_WIFI_IF";
	case ESP_ERR_WIFI_MODE:
		return "ESP_ERR_WIFI_MODE";
	case ESP_ERR_WIFI_STATE:
		return "ESP_ERR_WIFI_STATE";
	case ESP_ERR_WIFI_CONN:
		return "ESP_ERR_WIFI_CONN";
	case ESP_ERR_WIFI_NVS:
		return "ESP_ERR_WIFI_NVS";
	case ESP_ERR_WIFI_MAC:
		return "ESP_ERR_WIFI_MAC";
	case ESP_ERR_WIFI_SSID:
		return "ESP_ERR_WIFI_SSID";
	case ESP_ERR_WIFI_PASSWORD:
		return "ESP_ERR_WIFI_PASSWORD";
	case ESP_ERR_WIFI_TIMEOUT:
		return "ESP_ERR_WIFI_TIMEOUT";
	case ESP_ERR_WIFI_WAKE_FAIL:
		return "ESP_ERR_WIFI_WAKE_FAIL";
	}
	return "Unknown ESP_ERR error";
} // espToString


/**
 * Hack fix for ESP-IDF issue https://github.com/espressif/esp-idf/issues/83
 */
double __ieee754_remainder(double x, double y) {
	return x - y * floor(x/y);
} // __ieee754_remainder
