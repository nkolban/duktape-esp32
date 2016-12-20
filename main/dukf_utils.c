/**
 * Common utilities for the ESP32-Duktape framework.
 */
#ifdef ESP_PLATFORM
#include <esp_log.h>
#include <espfs.h>
#include <esp_system.h>
#else
#include <sys/mman.h>
#endif
#include "dukf_utils.h"
#include <stdio.h>
#include "logging.h"
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>

#include <duktape.h>
#include "duktape_utils.h"

#define MAX_RUN_AT_START (5)

LOG_TAG("dukf_utils");

// Number of scripts registered to run at the start.
static int g_runAtStartCount = 0;

// Array of scripts to run at start.
static char *g_runAtStartFileNames[MAX_RUN_AT_START];

/**
 * Log the heap value to the console.
 */
void dukf_log_heap(char *tag) {
#ifdef ESP_PLATFORM
	LOGD("heap=%d", esp_get_free_heap_size());
#endif
}


/**
 * Run the files that were registered to be run at startup.
 */
void dukf_runAtStart(duk_context *ctx) {
	LOGD("Running scripts registered to auto start:");
	int i;
	for (i=0;i<g_runAtStartCount; i++) {
		LOGD(" autostart: %s", g_runAtStartFileNames[i]);
		dukf_runFile(ctx, g_runAtStartFileNames[i]);
	}
} // dukf_runAtStart


/**
 * Load the named file and run it in a JS environment.
 */
void dukf_runFile(duk_context *ctx, const char *fileName) {
	LOGD(">> dukf_runFile: %s", fileName);
	size_t fileSize;
	const char *fileData = dukf_loadFile(fileName, &fileSize);
	if (fileData == NULL) {
		LOGE("<< dukf_runFile: Failed to load file");
		return;
	}
	// At this point we have the file in memory and we know its size.  Now we push it onto
	// the stack and run the script.
	duk_push_string(ctx, fileName);
	duk_compile_lstring_filename(ctx, 0, fileData, fileSize);
	//int rc = duk_peval_lstring(ctx, fileData, fileSize);
	int rc = duk_pcall(
		ctx,
		0 // Number of arguments
	);
	if (rc != 0) {
		esp32_duktape_log_error(ctx);
	}
	duk_pop(ctx);
	LOGD("<< dukf_runFile");
} // dukf_runFile


/**
 * Record the name of a file we wish to run at the start.
 */
void dukf_addRunAtStart(const char *fileName) {
	// Add the filename to the list of scripts to run at the start after first
	// checking that we haven't tried to remember too many.
	if (g_runAtStartCount < MAX_RUN_AT_START) {
		g_runAtStartFileNames[g_runAtStartCount] = (char *)fileName;
		g_runAtStartCount++;
	} else {
		LOGE("Can't run program %s at start ... too many already", fileName);
	}
} // dukf_addRunAtStart


#ifdef ESP_PLATFORM
/**
 * Load the named file and return the data and the size of it.
 * We load the file from the DUKF file system.
 */
const char *dukf_loadFile(const char *path, size_t *fileSize) {
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
  ESP_LOGD(tag, "duk_loadFile: Read file %s for size %d", path, *fileSize);
  return data;
} // dukf_loadFile

#else
/**
 * Load the named file and return the data and the size of it.
 * We load the file from the DUKF file system.
 */
#define DUKF_BASE_DIR "/home/kolban/esp32/esptest/apps/workspace/duktape/spiffs"
const char *dukf_loadFile(const char *path, size_t *fileSize) {

	char fileName[256];
	sprintf(fileName, "%s/%s", DUKF_BASE_DIR, path);
	int fd = open(fileName, O_RDONLY);
	if (fd < 0) {
		LOGE("open: %s - %d %s", path, errno, strerror(errno));
		return NULL;
	}
	struct stat statBuf;
	int rc = fstat(fd, &statBuf);
	if (rc < 0) {
		LOGE("open: %d %s", errno, strerror(errno));
		close(fd);
		return NULL;
	}

	char *data = mmap(NULL, statBuf.st_size, PROT_READ, MAP_SHARED, fd, 0);
	if (data == MAP_FAILED) {
		LOGE("mmap: %d %s", errno, strerror(errno));
		close(fd);
		return NULL;
	}
	close(fd);
  LOGD("duk_loadFile: Read file %s for size %d", path, (int)statBuf.st_size);
  *fileSize = statBuf.st_size;
	return data;
} // dukf_loadFile

#endif
