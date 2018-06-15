#include "sdkconfig.h"

#include <freertos/FreeRTOSConfig.h>
#include <driver/uart.h>
#include <esp_task.h>
#include <duktape.h>
#include <esp_log.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <esp_vfs.h>

#include "duktape_utils.h"
#include "logging.h"

LOG_TAG("module_serialvfs");
static duk_context *ctxsave = 0;
static int serialPort = -1;

#define MAX_OPEN_FILES 4
static struct f_d {
	char name[64];
	int size;
	int pos;
} files[MAX_OPEN_FILES] = {0};

static struct f_d *recover_f(int fd)
{
	if(fd<0 || fd>=MAX_OPEN_FILES) return 0;
	return files + fd;
}

static void send_request(void)
{
	const char *s = duk_json_encode(ctxsave, -1);
//	LOGI("JSON encoded: %s\n", s);
	int len = uart_write_bytes(serialPort, s, strlen(s)+1);
	len=len;
	duk_pop(ctxsave);
}

static int get_response(unsigned char *put, int len)
{
//	int numRead = uart_read_bytes(port, data, available, 0);

//LOGI("get response %d", len);

	unsigned char temp[16];
	int got = 0;
	while(got<2)
	{
		int t = uart_read_bytes(serialPort, temp+got, 2-got, 0);
//LOGI("  t = %d", t);
		if(t<0)
		{
			LOGE("Error read1");
			return -1;
		}
		got += t;
	}
	int avail = temp[0] + (temp[1]<<8);
	int res = 0;
	while(res < avail)
	{
		int toread = avail - res;
		int space = len-res;
		if(space>0) // still have space in buffer
		{
			if(toread>space) toread=space;
			got = uart_read_bytes(serialPort, put+res, toread, 0);
//LOGI("  got(1) = %d", got);
		} else // discard excess
		{
			if(toread > sizeof(temp)) toread=sizeof(temp);
			got = uart_read_bytes(serialPort, temp, toread, 0);
//LOGI("  got(2) = %d", got);
		}
		if(got<0)
		{
			LOGE("Error read2");
			return -1;
		}
		res += got;
	}
	return res<len ? res : len;
}

struct lowst {
	int size;
};

static int low_stat(const char *path, struct lowst *lowst)
{
	memset(lowst, 0, sizeof(*lowst));
	duk_push_object(ctxsave);
	duk_push_string(ctxsave, path);
	duk_put_prop_string(ctxsave, -2, "path");
	duk_push_string(ctxsave, "stat");
	duk_put_prop_string(ctxsave, -2, "cmd");
	send_request();
	unsigned char statbuff[16];
	int got = get_response(statbuff, sizeof(statbuff));
//	LOGI("got = %d", got);
	unsigned short le2(unsigned char *p) {return p[0] | (p[1]<<8);}
	unsigned int le4(unsigned char *p) {return le2(p) | (le2(p+2)<<16);}
	if(got>=4)
	{
		lowst->size = le4(statbuff);
		return 0;
	}
	return -1;
}

static int myfs_open(const char *path, int flags, int mode)
{
//	LOGI("open %s", path);
	int i;
	for(i=0;i<MAX_OPEN_FILES;++i)
	{
		if(files[i].name[0]==0)
			break;
	}
	if(i==MAX_OPEN_FILES)
	{
		LOGE("Too many open files");
		return -1;
	}
	struct f_d *f = files + i;
	struct lowst lowst;
	if(low_stat(path, &lowst) == 0)
	{
		memset(f, 0, sizeof(*f));
		snprintf(f->name, sizeof(f->name), "%s", path);
		f->size = lowst.size;
	} else
		return -1;
	return i;
}

static int myfs_close(int fd)
{
//	LOGI("close %d", fd);
	struct f_d *f = recover_f(fd);
	if(!f) return -1;
	if(f->name[0] == 0) return -1;
	memset(f, 0, sizeof(*f));
	return 0;
}
static ssize_t myfs_write(int fd, const void * data, size_t size)
{
//	LOGI("write %d", fd);
	return 0;
}

static ssize_t myfs_read(int fd, void * data, size_t size)
{
//	LOGI("read %d", fd);
	struct f_d *f = recover_f(fd);
	if(!f) return -1;
	if(f->name[0] == 0) return -1;

	duk_push_object(ctxsave);
	duk_push_string(ctxsave, f->name);
	duk_put_prop_string(ctxsave, -2, "path");
	duk_push_string(ctxsave, "read");
	duk_put_prop_string(ctxsave, -2, "cmd");
	duk_push_int(ctxsave, f->pos);
	duk_put_prop_string(ctxsave, -2, "pos");
	duk_push_int(ctxsave, size);
	duk_put_prop_string(ctxsave, -2, "len");
	send_request();
	int got = get_response(data, size);
	if(got>0) f->pos += got;
	return got;
}

static int myfs_fstat(int fd, struct stat *st)
{
//	LOGI("fstat %d", fd);
	struct f_d *f = recover_f(fd);
	if(f->name[0] == 0) return -1;
	st->st_size = f->size;
	st->st_mode = S_IRWXU | S_IRWXG | S_IRWXO | S_IFREG;
	st->st_mtime = 0;
	st->st_atime = 0;
	st->st_ctime = 0;
	return 0;
}

static int myfs_stat(const char *path, struct stat *st)
{
//	LOGI("stat %s", path);
	struct lowst lowst;
	if(low_stat(path, &lowst) == 0)
	{
		st->st_size = lowst.size;
		st->st_mode = S_IRWXU | S_IRWXG | S_IRWXO | S_IFREG;
		st->st_mtime = 0;
		st->st_atime = 0;
		st->st_ctime = 0;
		return 0;
	}
	return -1;
}


static duk_ret_t js_serialvfs_init(duk_context *ctx) {
	LOGD(">> init_serialvfs");
	esp_vfs_t myfs = {
	    .flags = ESP_VFS_FLAG_DEFAULT,
	    .write = &myfs_write,
	    .open = &myfs_open,
	    .fstat = &myfs_fstat,
	    .stat = &myfs_stat,
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

	duk_get_prop_string(ctx, -1, "serial");
	if (!duk_is_number(ctx, -1)) {
		duk_error(ctx, 1, "Invalid serial number");
	}
	serialPort = duk_get_int(ctx, -1);
	duk_pop(ctx);
	LOGI("serialPort:%d", serialPort);

//	uart_write_bytes(serialPort, "From module_serialvfs.c\r\n", 25);
//	int numRead = uart_read_bytes(port, data, available, 0);



// used main/module_os.c as guide (dashxdr was here 20180614)

	ret = esp_vfs_register(mount, &myfs, NULL);
	if(ret != ESP_OK)
	{
		LOGE("Failed to register netvfs");
		return 0;
	}
	LOGI("Directory mounted");
	ctxsave = ctx;
	return 0;
} // init_netvfs


/**
 * Add native methods to the NetVFS object.
 * [0] - SerialVFS Object
 */
duk_ret_t ModuleSerialVFS(duk_context *ctx) {
	ADD_FUNCTION("init", js_serialvfs_init, 1);
	return 0;
} // ModuleNetVFS
