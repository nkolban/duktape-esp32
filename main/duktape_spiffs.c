/**
 * SPIFFs documentation can be found at https://github.com/pellepl/spiffs
 */
#include "duktape_spiffs.h"
#include <spiffs.h>
#include <esp_spiffs.h>
#include <esp_log.h>
#include <esp_vfs.h>
#include <fcntl.h>
#include <errno.h>
#include <duktape.h>
#include "sdkconfig.h"

#define DUKTAPE_VFS_MOUNTPOINT "/spiffs"

static char tag[] = "duktape_spiffs";

// SPIFFS storage areas.
static spiffs fs;
#define LOG_PAGE_SIZE       256
static uint8_t spiffs_work_buf[LOG_PAGE_SIZE*2];
static uint8_t spiffs_fds[32*4];
static uint8_t spiffs_cache_buf[(LOG_PAGE_SIZE+32)*4];


static void spiffs_registerVFS(char *mountPoint, spiffs *fs);

static const char *typeToString(spiffs_obj_type type) {
	switch(type) {
	case SPIFFS_TYPE_DIR:
		return "Dir";
	case SPIFFS_TYPE_FILE:
		return "File";
	case SPIFFS_TYPE_HARD_LINK:
		return "Hard link";
	case SPIFFS_TYPE_SOFT_LINK:
		return "Soft link";
	default:
		return "Unknown";
	}
} // typeToString

void esp32_duktape_spiffs_write(char *fileName, uint8_t *data, int length) {
	spiffs_file fh = SPIFFS_open(&fs, fileName, SPIFFS_O_CREAT | SPIFFS_O_RDWR | SPIFFS_O_TRUNC ,0);
	SPIFFS_write(&fs, fh, data, length);
	SPIFFS_close(&fs, fh);
}


/**
 * Create a JS array on the value stack which contains objects.  Each
 * object represents one file entry.  Each object contains:
 * {
 *    name: <file name>
 *    size: <file size>
 * }
 */
void esp32_duktape_dump_spiffs_array(duk_context *ctx) {
	spiffs_DIR dir;
	struct spiffs_dirent dirEnt;
	const char rootPath[] = "/";

	ESP_LOGD(tag, ">> esp32_duktape_dump_spiffs_json: %s", rootPath);

	duk_push_array(ctx);

	if (SPIFFS_opendir(&fs, rootPath, &dir) == NULL) {
		ESP_LOGD(tag, "Unable to open %s dir", rootPath);
		return;
	}
	int arrayIndex = 0;
	while(SPIFFS_readdir(&dir, &dirEnt) != NULL) {
		int len = strlen((char *)dirEnt.name);
		// Skip files that end with "/."
		if (len>=2 && strcmp((char *)(dirEnt.name + len -2), "/.") == 0) {
			continue;
		}
		duk_push_object(ctx);
		duk_push_string(ctx, (char *)dirEnt.name);
		duk_put_prop_string(ctx, -2, "name");
		duk_push_int(ctx, dirEnt.size);
		duk_put_prop_string(ctx, -2, "size");
		duk_put_prop_index(ctx, -2, arrayIndex);
		arrayIndex++;
	}
	ESP_LOGD(tag, "<< dump_fs");
	return;
} // esp32_duktape_dump_spiffs_json


void esp32_duktape_dump_spiffs() {
	spiffs_DIR dir;
	struct spiffs_dirent dirEnt;
	const char rootPath[] = "/";

	ESP_LOGD(tag, ">> dump_fs: %s", rootPath);

	if (SPIFFS_opendir(&fs, rootPath, &dir) == NULL) {
		ESP_LOGD(tag, "Unable to open %s dir", rootPath);
		return;
	}

	while(1) {
		if (SPIFFS_readdir(&dir, &dirEnt) == NULL) {
			ESP_LOGD(tag, "<< dump_fs");
			return;
		}
		ESP_LOGD(tag, "name=%s, id=%x, type=%s, size=%d", dirEnt.name, dirEnt.obj_id, typeToString(dirEnt.type), dirEnt.size)
	}
} // dump_fs


/**
 * Mount the spiffs file system.  Once mounted, register it as a
 * virtual file system.
 */
void esp32_duktape_spiffs_mount() {
	spiffs_config cfg;
	int rc;
	cfg.phys_size = 512*1024; // use all spi flash
	cfg.phys_addr = 2*1024*1024 - cfg.phys_size; // start spiffs at start of spi flash
	cfg.phys_erase_block = 65536; // according to datasheet
	cfg.log_block_size = 65536; // let us not complicate things
	cfg.log_page_size = LOG_PAGE_SIZE; // as we said

	cfg.hal_read_f = esp32_spi_flash_read;
	cfg.hal_write_f = esp32_spi_flash_write;
	cfg.hal_erase_f = esp32_spi_flash_erase;

	ESP_LOGD(tag, "Loading SPIFFS at address 0x%x", cfg.phys_addr);
	rc = SPIFFS_mount(&fs,
		&cfg,
		spiffs_work_buf,
		spiffs_fds,
		sizeof(spiffs_fds),
		spiffs_cache_buf,
		sizeof(spiffs_cache_buf),
		0);
	if (rc != 0) {
		ESP_LOGE(tag, "Error: rc from spiffs mount = %d", rc);
	} else {
		spiffs_registerVFS(DUKTAPE_VFS_MOUNTPOINT, &fs);
	}
} // esp32_duktape_spiffs_mount


/*
static int spiffsErrMap(spiffs *fs) {
	int errorCode = SPIFFS_errno(fs);
	switch (errorCode) {
		case SPIFFS_ERR_FULL:
			return ENOSPC;
		case SPIFFS_ERR_NOT_FOUND:
			return ENOENT;
		case SPIFFS_ERR_FILE_EXISTS:
			return EEXIST;
		case SPIFFS_ERR_NOT_A_FILE:
			return EBADF;
		case SPIFFS_ERR_OUT_OF_FILE_DESCS:
			return ENFILE;
		default: {
			ESP_LOGE(tag, "We received SPIFFs error code %d but didn't know how to map to an errno", errorCode);
			return ENOMSG;
		}
	}
} // spiffsErrMap
*/


/**
 * Log the flags that are specified in an open() POSIX call.
 */
static void logOpenFlags(int flags) {
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

	int openMode = flags & 3;
	if (openMode == O_RDONLY) {
		ESP_LOGD(tag, "- O_RDONLY");
	}
	if (openMode == O_WRONLY) {
		ESP_LOGD(tag, "- O_WRONLY");
	}
	if (openMode == O_RDWR) {
		ESP_LOGD(tag, "- O_RDWR");
	}
} // End of logFlags


static size_t vfs_write(void *ctx, int fd, const void *data, size_t size) {
	ESP_LOGI(tag, ">> write fd=%d, data=0x%lx, size=%d", fd, (unsigned long)data, size);
	spiffs *fs = (spiffs *)ctx;
	size_t retSize = SPIFFS_write(fs, (spiffs_file)fd, (void *)data, size);
	return retSize;
} // vfs_write


static off_t vfs_lseek(void *ctx, int fd, off_t offset, int whence) {
	ESP_LOGI(tag, ">> lseek fd=%d, offset=%d, whence=%d", fd, (int)offset, whence);
	spiffs *fs = (spiffs *)ctx;
	int rc = SPIFFS_lseek(fs, fd, offset, whence);
	return rc;
} // vfs_lseek


static ssize_t vfs_read(void *ctx, int fd, void *dst, size_t size) {
	ESP_LOGI(tag, ">> read fd=%d, dst=0x%lx, size=%d", fd, (unsigned long)dst, size);
	spiffs *fs = (spiffs *)ctx;
	ssize_t retSize = SPIFFS_read(fs, (spiffs_file)fd, dst, size);
	ESP_LOGD(tag, "vfs_read(SPIFFS): We read %d", retSize);
	return retSize;
} // vfs_read


/**
 * Open the file specified by path.  The flags contain the instructions
 * on how the file is to be opened.  For example:
 *
 * O_CREAT  - Create the named file.
 * O_TRUNC  - Truncate (empty) the file.
 * O_RDONLY - Open the file for reading only.
 * O_WRONLY - Open the file for writing only.
 * O_RDWR   - Open the file for reading and writing.
 * O_APPEND - Append to the file.
 *
 * The mode are access mode flags.
 */
static int vfs_open(void *ctx, const char *path, int flags, int accessMode) {
	ESP_LOGI(tag, ">> open path=%s, flags=0x%x, accessMode=0x%x", path, flags, accessMode);
	logOpenFlags(flags);
	spiffs *fs = (spiffs *)ctx;
	int spiffsFlags = 0;

	// The openMode is encoded in the 1st 2 bits as:
	// 0 - 00 - RDONLY
	// 1 - 01 - WRONLY
	// 2 - 10 - RDWR
	// 3 - 11 - Undefined

	int openMode = flags & 3;
	if (openMode == O_RDONLY) {
		spiffsFlags |= SPIFFS_O_RDONLY;
	} else if (openMode == O_RDWR) {
		spiffsFlags |= SPIFFS_O_RDWR;
	} else if (openMode == O_WRONLY) {
		spiffsFlags |= SPIFFS_O_WRONLY;
	}

	if (flags & O_CREAT) {
		spiffsFlags |= SPIFFS_O_CREAT;
	}
	if (flags & O_TRUNC) {
		spiffsFlags |= SPIFFS_O_TRUNC;
	}
	if (flags & O_APPEND) {
		spiffsFlags |= SPIFFS_O_APPEND;
	}
	int rc = SPIFFS_open(fs, path, spiffsFlags, accessMode);
	return rc;
} // vfs_open


static int vfs_close(void *ctx, int fd) {
	ESP_LOGI(tag, ">> close fd=%d", fd);
	spiffs *fs = (spiffs *)ctx;
	int rc = SPIFFS_close(fs, (spiffs_file)fd);
	return rc;
} // vfs_close


static int vfs_fstat(void *ctx, int fd, struct stat *st) {
	spiffs_stat spiffsStat;
	ESP_LOGI(tag, ">> fstat fd=%d", fd);
	spiffs *fs = (spiffs *)ctx;
	SPIFFS_fstat(fs, fd, &spiffsStat);
	st->st_size = spiffsStat.size;
	return 1;
} // vfs_fstat


static int vfs_stat(void *ctx, const char *path, struct stat *st) {
	ESP_LOGI(tag, ">> stat path=%s", path);
	spiffs_stat spiffsStat;
	spiffs *fs = (spiffs *)ctx;
	SPIFFS_stat(fs, path, &spiffsStat);
	st->st_size = spiffsStat.size;
	return 1;
} // vfs_stat


static int vfs_unlink(void *ctx, const char *path) {
	ESP_LOGI(tag, ">> unlink path=%s", path);
	spiffs *fs = (spiffs *)ctx;
	SPIFFS_remove(fs, path);
	return 0;
} // vfs_unlink


static int vfs_link(void *ctx, const char *oldPath, const char *newPath) {
	ESP_LOGI(tag, ">> link oldPath=%s, newPath=%s", oldPath, newPath);
	return 0;
} // vfs_link



static int vfs_rename(void *ctx, const char *oldPath, const char *newPath) {
	ESP_LOGI(tag, ">> rename oldPath=%s, newPath=%s", oldPath, newPath);
	spiffs *fs = (spiffs *)ctx;
	int rc = SPIFFS_rename(fs, oldPath, newPath);
	return rc;
} // vfs_rename


/**
 * Register the VFS at the specified mount point.
 * The callback functions are registered to handle the
 * different functions that may be requested against the
 * VFS.
 */
static void spiffs_registerVFS(char *mountPoint, spiffs *fs) {
	esp_vfs_t vfs;
	esp_err_t err;

	vfs.fd_offset = 0;
	vfs.flags = ESP_VFS_FLAG_CONTEXT_PTR;
	vfs.write_p  = vfs_write;
	vfs.lseek_p  = vfs_lseek;
	vfs.read_p   = vfs_read;
	vfs.open_p   = vfs_open;
	vfs.close_p  = vfs_close;
	vfs.fstat_p  = vfs_fstat;
	vfs.stat_p   = vfs_stat;
	vfs.link_p   = vfs_link;
	vfs.unlink_p = vfs_unlink;
	vfs.rename_p = vfs_rename;

	err = esp_vfs_register(mountPoint, &vfs, (void *)fs);
	if (err != ESP_OK) {
		ESP_LOGE(tag, "esp_vfs_register: err=%d", err);
	}
} // spiffs_registerVFS

