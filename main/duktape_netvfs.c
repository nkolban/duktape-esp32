#include "duktape_netvfs.h"
#include <esp_log.h>
#include <fcntl.h>
#include <errno.h>
#include <duktape.h>
#include <dirent.h>
#include "sdkconfig.h"
#include "logging.h"

#include <esp_vfs.h>

LOG_TAG("duktape_netvfs");

static int myfs_open(const char *path, int flags, int mode)
{
	LOGI("open %s", path);
	return 0;
}
static int myfs_close(int fd)
{
	LOGI("close %d", fd);
	return 0;
}
static ssize_t myfs_write(int fd, const void * data, size_t size)
{
	LOGI("write %d", fd);
	return 0;
}

static ssize_t myfs_read(int fd, void * data, size_t size)
{
	LOGI("read %d", fd);
	return 0;
}

static int myfs_fstat(int fd, struct stat *st)
{
	LOGI("fstat %d", fd);
	return 0;
}

void esp32_duktape_netvfs_mount() {
	LOGI("mount");
	esp_vfs_t myfs = {
	    .flags = ESP_VFS_FLAG_DEFAULT,
	    .write = &myfs_write,
	    .open = &myfs_open,
	    .fstat = &myfs_fstat,
	    .close = &myfs_close,
	    .read = &myfs_read,
	};
	esp_err_t ret;

	ret = esp_vfs_register("/netvfs", &myfs, NULL);
	if(ret != ESP_OK)
		LOGE("Failed to register netvfs");
}
