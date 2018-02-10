/**
 * SPIFFs documentation can be found at https://github.com/pellepl/spiffs
 */
#include "duktape_spiffs.h"
#include <esp_log.h>
#include <fcntl.h>
#include <errno.h>
#include <duktape.h>
#include <dirent.h>
#include "sdkconfig.h"
#include "logging.h"

LOG_TAG("duktape_spiffs");

#define DUKTAPE_SPIFFS_MOUNTPOINT "/spiffs"

static const char *typeToString(char type) {
    switch(type) {
    case DT_DIR:
        return "Dir";
    case DT_REG:
        return "File";
    default:
        return "Unknown";
    }
} // typeToString

/**
 * Create a JS array on the value stack which contains objects.  Each
 * object represents one file entry.  Each object contains:
 * {
 *    name: <file name>
 *    size: <file size>
 * }
 */
void esp32_duktape_dump_spiffs_array(duk_context *ctx) {
    DIR* dir = opendir(DUKTAPE_SPIFFS_MOUNTPOINT);
    struct dirent *dirEnt;
    const char rootPath[] = "/";

    LOGD(">> esp32_duktape_dump_spiffs_array: rootPath=\"%s\"", rootPath);

    duk_push_array(ctx);

    if (dir == NULL) {
        LOGD("Unable to open %s dir", rootPath);
    } else {
        int arrayIndex = 0;
        while((dirEnt = readdir(dir))) {
            int len = strlen((char *)dirEnt->d_name);
            // Skip files that end with "/."
            if (len>=2 && strcmp((char *)(dirEnt->d_name + len -2), "/.") == 0) {
                continue;
            }
            duk_push_object(ctx);
            duk_push_string(ctx, (char *)dirEnt->d_name);
            duk_put_prop_string(ctx, -2, "name");
            duk_put_prop_index(ctx, -2, arrayIndex);
            arrayIndex++;
        }
        closedir(dir);
    }
    LOGD("<< esp32_duktape_dump_spiffs_array");
    return;
} // esp32_duktape_dump_spiffs_json


void esp32_duktape_dump_spiffs() {
    DIR* dir = opendir(DUKTAPE_SPIFFS_MOUNTPOINT);
    struct dirent *dirEnt;
    const char rootPath[] = "/";
    LOGD(">> dump_fs: %s", rootPath);
    if (dir == NULL) {
        LOGD("Unable to open %s dir", rootPath);
    } else {
        while ((dirEnt = readdir(dir))) {
            LOGD("name=%s, id=%x, type=%s", dirEnt->d_name, dirEnt->d_ino, typeToString(dirEnt->d_type));
        }
        closedir(dir);
    }
    LOGD("<< dump_fs");
} // dump_fs


/**
 * Mount & register virtual filesystem.
 */
void esp32_duktape_spiffs_mount() {
    esp_vfs_spiffs_conf_t conf = {
      .base_path = DUKTAPE_SPIFFS_MOUNTPOINT,
      .partition_label = NULL,
      .max_files = 5,
      .format_if_mount_failed = false
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            LOGE("Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            LOGE("Failed to find SPIFFS partition");
        } else {
            LOGE("Failed to initialize SPIFFS (%d)", ret);
        }
        return;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK) {
        LOGE("Failed to get SPIFFS partition information");
    } else {
        LOGI("Partition size: total: %d, used: %d", total, used);
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
