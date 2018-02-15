#include "sdkconfig.h"

#include <freertos/FreeRTOSConfig.h>
#include <esp_task.h>
#include <duktape.h>
#include <esp_log.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <esp_vfs.h>

#include "duktape_utils.h"
#include "logging.h"

LOG_TAG("module_netvfs");

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
}


static duk_ret_t js_netvfs_init(duk_context *ctx) {
	LOGD(">> init_netvfs");
	esp_vfs_t myfs = {
	    .flags = ESP_VFS_FLAG_DEFAULT,
	    .write = &myfs_write,
	    .open = &myfs_open,
	    .fstat = &myfs_fstat,
	    .close = &myfs_close,
	    .read = &myfs_read,
	};
	esp_err_t ret;

	if (!duk_is_object(ctx, -1)) {
		duk_error(ctx, 1, "Missing config object");
	}

	duk_get_prop_string(ctx, -1, "mount");
	if (!duk_is_string(ctx, -1)) {
		duk_error(ctx, 1, "Invalid mount");
	}
	const char *mount = duk_get_string(ctx, -1);
	duk_pop(ctx);
	LOGI("mount:%s", mount);

	duk_get_prop_string(ctx, -1, "server");
	if (!duk_is_string(ctx, -1)) {
		duk_error(ctx, 1, "Invalid server");
	}
	const char *server = duk_get_string(ctx, -1);
	duk_pop(ctx);
	LOGI("server:%s", server);

	duk_get_prop_string(ctx, -1, "port");
	if (!duk_is_number(ctx, -1)) {
		duk_error(ctx, 1, "Invalid port");
	}
	int port = duk_get_number(ctx, -1);
	duk_pop(ctx);
	LOGI("port:%d", port);


	ret = esp_vfs_register(mount, &myfs, NULL);
	if(ret != ESP_OK)
		LOGE("Failed to register netvfs");
	return 0;
} // init_netvfs


/**
 * Add native methods to the NetVFS object.
 * [0] - NetVFS Object
 */
duk_ret_t ModuleNetVFS(duk_context *ctx) {
	ADD_FUNCTION("init", js_netvfs_init, 1);
	return 0;
} // ModuleNetVFS
