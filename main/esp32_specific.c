/**
 * ESP32 specific functions supporting the Duktape project.
 */

#include <esp_vfs.h>
#include <esp_log.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <espfs.h>
#include "sdkconfig.h"



static char tag[] = "esp32_specific";

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


char *esp32_loadFileESPFS(char *path, size_t *fileSize) {
	EspFsFile *fh = espFsOpen((char *)path);
	if (fh == NULL) {
		ESP_LOGD(tag, " Failed to open file %s", path);
		return NULL;
	}
  char *data;
  espFsAccess(fh, (void **)&data, fileSize);
  espFsClose(fh);
  // Note ... because data is mapped in memory from flash ... it will be good
  // past the file close.
  ESP_LOGD(tag, "esp32_loadFileESPFS: Read file %s for size %d", path, *fileSize);
  return data;
} // esp32_loadFileESPFS

/**
 * Log the flags that are specified in an open() call.
 */
static void logFlags(int flags) {
	ESP_LOGD(tag, "flags:");
	if (flags & O_APPEND) {
		ESP_LOGD(tag, "- O_APPEND");
	}
	if (flags & O_CREAT) {
		ESP_LOGD(tag, "- O_CREAT");
	}
	if (flags & O_TRUNC) {
		ESP_LOGD(tag, "- O_TRUNC");
	}
	if (flags & O_RDONLY) {
		ESP_LOGD(tag, "- O_RDONLY");
	}
	if (flags & O_WRONLY) {
		ESP_LOGD(tag, "- O_WRONLY");
	}
	if (flags & O_RDWR) {
		ESP_LOGD(tag, "- O_RDWR");
	}
} // End of logFlags


static size_t vfs_write(int fd, const void *data, size_t size) {
	ESP_LOGI(tag, ">> write fd=%d, data=0x%lx, size=%d", fd, (unsigned long)data, size);
	return 0;
}


static off_t vfs_lseek(int fd, off_t offset, int whence) {
	ESP_LOGI(tag, ">> lseek fd=%d, offset=%d, whence=%d", fd, (int)offset, whence);
	return 0;
}


static ssize_t vfs_read(int fd, void *dst, size_t size) {
	ESP_LOGI(tag, ">> read fd=%d, dst=0x%lx, size=%d", fd, (unsigned long)dst, size);
	return 0;
}


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
static int vfs_open(const char *path, int flags, int accessMode) {
	ESP_LOGI(tag, ">> open path=%s, flags=0x%x, accessMode=0x%x", path, flags, accessMode);
	logFlags(flags);
	return -1;
}


static int vfs_close(int fd) {
	ESP_LOGI(tag, ">> close fd=%d", fd);
	return 0;
}


static int vfs_fstat(int fd, struct stat *st) {
	ESP_LOGI(tag, ">> fstat fd=%d", fd);
	return 0;
}


static int vfs_stat(const char *path, struct stat *st) {
	ESP_LOGI(tag, ">> stat path=%s", path);
	return 0;
}


static int vfs_link(const char *oldPath, const char *newPath) {
	ESP_LOGI(tag, ">> link oldPath=%s, newPath=%s", oldPath, newPath);
	return 0;
}


static int vfs_rename(const char *oldPath, const char *newPath) {
	ESP_LOGI(tag, ">> rename oldPath=%s, newPath=%s", oldPath, newPath);
	return 0;
}


/**
 * Register the VFS at the specified mount point.
 * The callback functions are registered to handle the
 * different functions that may be requested against the
 * VFS.
 */
void registerTestVFS(char *mountPoint) {

} // End of registerTestVFS


void setupVFS() {
	ESP_LOGD(tag, ">> setupVFS");
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
		ESP_LOGE(tag, "setupVFS: esp_vfs_register: err=%d", err);
	}
	ESP_LOGD(tag, "<< setupVFS");
} // setupVFS





/**
 * Open a file that is actually a web page.  Here we retrieve the content
 * of a web page against an HTTP server where the server is supplied by
 * the path.
 */
static int web_vfs_open(const char *path, int flags, int accessMode) {
	return 0;
} // web_vfs_open


static int web_vfs_close(int fd) {
	ESP_LOGI(tag, ">> web_vfs_close fd=%d", fd);
	if (fd == -1) {
		return 0;
	}
	file_record_t *pFileRecord = getFilerecord(fd);
	if (pFileRecord == NULL) {
		ESP_LOGD(tag, "File record for fd=%d not found", fd);
		return -1;
	}
	if (pFileRecord->inUse == 0) {
		ESP_LOGD(tag, "Unexpected state ... closing fd=%d but it appear to be free", fd);
		return -1;
	}
	free(pFileRecord->data);
	pFileRecord->inUse = 0;
	return 0;
} // web_vfs_close


/**
 * Read the web context into the buffer passed in.  The web content structure
 * we already have contains:
 * * data - The pointer to the web retrieved data.
 * * size - The size of the data retrieved.
 * * fptr - The file pointer of where we currently are within the retrieved data.
 */
static ssize_t web_vfs_read(
		int fd, // The file descriptor to read the data from.
		void *dst, // The buffer where we will write the read data.
		size_t bufferSize // The number of bytes available in the buffer.
	) {
	ssize_t retSize;
	file_record_t *pFileRecord = &file_records[0];
	ESP_LOGI(tag, ">> web_vfs_read fd=%d, dst=0x%lx, bufferSize=%d", fd, (unsigned long)dst, bufferSize);
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

	ESP_LOGD(tag, "<< web_vfs_read  retSize = %d", retSize);
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
		ESP_LOGE(tag, "createWebVFS: esp_vfs_register: err=%d", err);
	}
	int i;
	for (i=0; i<MAX_WEB_FILE_RECORDS; i++) {
		file_records[i].inUse = 0;
		file_records[i].fd = i;
	}
} // createWebVFS
