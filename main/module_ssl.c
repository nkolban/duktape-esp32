#include <assert.h>
#include <errno.h>
#include <lwip/sockets.h>
#include <mbedtls/platform.h> // Has to come early

#include <mbedtls/ctr_drbg.h>
#include <mbedtls/debug.h>
#include <mbedtls/entropy.h>
#include <mbedtls/error.h>
#include <mbedtls/net.h>
#include <mbedtls/ssl.h>
#include <string.h>

#include "duktape_utils.h"
#include "logging.h"
#include "module_ssl.h"

LOG_TAG("ssl");

static void debug_log(
	void *context,
	int level,
	const char *file,
	int line,
	const char *message) {
	// mbedtls messages seem to add a newline, let's remove that.
	char *ptr = (char *)message;
	if (ptr[strlen(ptr)-1] == '\n') {
		ptr[strlen(ptr)-1] = ' ';
	}
	LOGD("%s:%04d: %s", file, line, message);
} // debug_log


/*
 * Provide support for SSL (secure sockets layer) from within the ESP32-Duktape environment.
 */
typedef struct {
  mbedtls_x509_crt cacert;
  mbedtls_ssl_config conf;
  mbedtls_ctr_drbg_context ctr_drbg;
  mbedtls_entropy_context entropy;
  mbedtls_ssl_context ssl;
  int fd;
} dukf_ssl_context_t;

const static char *pers = "ssl_client1";

static int ssl_send(void *ctx, const unsigned char *buf, size_t len) {
	LOGD(">> ssl_send");
	assert(ctx != NULL);
	assert(buf != NULL);
	assert(len > 0);
	dukf_ssl_context_t *dukf_ssl_context = (dukf_ssl_context_t *)ctx;

	int rc = send(dukf_ssl_context->fd, buf, len, 0);
	if (rc == -1) {
		LOGE("send(): errno=%s", strerror(errno));
	}
	LOGD("<< ssl_send: fd=%d, rc=%d",dukf_ssl_context->fd, rc);
	return rc;
} // ssl_send

static int ssl_recv(void *ctx, unsigned char *buf, size_t len) {
	LOGD(">> ssl_recv: max_length=%d", len);
	dukf_ssl_context_t *dukf_ssl_context = (dukf_ssl_context_t *)ctx;
	int rc = recv(dukf_ssl_context->fd, buf, len, 0);
	LOGD("<< ssl_recv: fd=%d, rc=%d", dukf_ssl_context->fd, rc);
	return rc;
} // ssl_recv

static dukf_ssl_context_t *create_dukf_ssl_context(const char *hostname, int fd) {
	char errortext[256];
	dukf_ssl_context_t *dukf_ssl_context;

	dukf_ssl_context = malloc(sizeof(dukf_ssl_context_t));
	mbedtls_ssl_init(&dukf_ssl_context->ssl);
	mbedtls_ssl_config_init(&dukf_ssl_context->conf);
	mbedtls_x509_crt_init(&dukf_ssl_context->cacert);
	mbedtls_ctr_drbg_init(&dukf_ssl_context->ctr_drbg);
	mbedtls_entropy_init(&dukf_ssl_context->entropy);

	mbedtls_ssl_conf_dbg(&dukf_ssl_context->conf, debug_log, NULL);
	dukf_ssl_context->fd = fd;

  int rc = mbedtls_ctr_drbg_seed(
     &dukf_ssl_context->ctr_drbg,
     mbedtls_entropy_func, &dukf_ssl_context->entropy, (const unsigned char *) pers, strlen(pers));
  if (rc != 0) {
     LOGE(" failed\n  ! mbedtls_ctr_drbg_seed returned %d", rc);
     return NULL;
  }

  rc = mbedtls_ssl_config_defaults(
		&dukf_ssl_context->conf,
		MBEDTLS_SSL_IS_CLIENT,
		MBEDTLS_SSL_TRANSPORT_STREAM,
		MBEDTLS_SSL_PRESET_DEFAULT);
	if (rc != 0) {
		 LOGE("mbedtls_ssl_config_defaults returned %d", rc);
		 return NULL;
	}

	mbedtls_ssl_conf_authmode(&dukf_ssl_context->conf, MBEDTLS_SSL_VERIFY_NONE);

	mbedtls_ssl_conf_rng(&dukf_ssl_context->conf, mbedtls_ctr_drbg_random, &dukf_ssl_context->ctr_drbg);

	rc = mbedtls_ssl_setup(&dukf_ssl_context->ssl, &dukf_ssl_context->conf);
	if (rc != 0) {
		 mbedtls_strerror(rc, errortext, sizeof(errortext));
		 LOGE("error from mbedtls_ssl_setup: %d - %x - %s\n", rc, rc, errortext);
		 return NULL;
	}

	rc = mbedtls_ssl_set_hostname(&dukf_ssl_context->ssl, hostname);
	if (rc) {
		 mbedtls_strerror(rc, errortext, sizeof(errortext));
		 LOGE("error from mbedtls_ssl_set_hostname: %s %d - %x - %s", hostname, rc, rc, errortext);
		 return NULL;
	}

	mbedtls_ssl_set_bio(&dukf_ssl_context->ssl, dukf_ssl_context, ssl_send, ssl_recv, NULL);
	return dukf_ssl_context;
} // create_ssl_socket


static void free_dukf_ssl_context(dukf_ssl_context_t *dukf_ssl_context) {
  mbedtls_ssl_free(&dukf_ssl_context->ssl);
  mbedtls_ssl_config_free(&dukf_ssl_context->conf);
  mbedtls_ctr_drbg_free(&dukf_ssl_context->ctr_drbg);
  mbedtls_entropy_free(&dukf_ssl_context->entropy);
  free(dukf_ssl_context);
} // free_ssl_socket

/*
 * [0] - hostname
 * [1] - socket fd
 */
static duk_ret_t js_ssl_create_dukf_ssl_context(duk_context *ctx) {
	const char *hostname = duk_get_string(ctx, -2);
	int fd = duk_get_int(ctx, -1);
	LOGD(">> js_ssl_create_dukf_ssl_context: hostname=\"%s\", fd=%d", hostname, fd);
	dukf_ssl_context_t *dukf_ssl_context = create_dukf_ssl_context(hostname, fd);
	duk_push_pointer(ctx, dukf_ssl_context);
	LOGD("<< js_ssl_create_dukf_ssl_context");
	return 1;
} // js_ssl_createSSLSocket

/*
 * Set the mbedtls sebug threshold.
 * 0 – no debug
 * 1 – error
 * 2 – state change
 * 3 – informational
 * 4 – verbose
 * [0] - Debug level
 */
static duk_ret_t js_ssl_debug_set_threshold(duk_context *ctx) {
	LOGD(">> js_ssl_debug_set_threshold");
	int threshold = duk_get_int(ctx, -1);
	if (threshold < 0 || threshold > 4) {
		LOGE("bad threshold");
	} else {
		mbedtls_debug_set_threshold(threshold);
	}
	LOGD("<< js_ssl_debug_set_threshold");
	return 0;
} // js_ssl_debug_set_threshold


/*
 * [0] - ptr to ssl_socket
 */
static duk_ret_t js_ssl_free_dukf_ssl_context(duk_context *ctx) {
	LOGD(">> js_ssl_free");
	dukf_ssl_context_t *dukf_ssl_context = duk_get_pointer(ctx, -1);
	if (dukf_ssl_context != NULL) {
		free_dukf_ssl_context(dukf_ssl_context);
	} else {
		LOGE("No ssl_socket passed.");
	}
	LOGD("<< js_ssl_free");
	return 0;
} // js_ssl_free


/*
 * [0] - dukf_ssl_context_t - pointer
 * [1] - data to read - buffer
 */
static duk_ret_t js_ssl_read(duk_context *ctx) {
	char errortext[256];
	LOGD(">> js_ssl_read");
	dukf_ssl_context_t *dukf_ssl_context = duk_get_pointer(ctx, -2);
	size_t len;
	uint8_t *buf = duk_get_buffer_data(ctx, -1, &len);
	int rc = mbedtls_ssl_read(&dukf_ssl_context->ssl, buf, len);
	if (rc < 0) {
		 mbedtls_strerror(rc, errortext, sizeof(errortext));
		 LOGE("error from mbedtls_ssl_read: %d - %x - %s", rc, rc, errortext);
		 duk_push_nan(ctx);
	} else {
		duk_push_int(ctx, rc);
	}
	LOGD("<< js_ssl_read: rc=%d", rc);
	return 1;
} // js_ssl_read


/*
 * [0] - dukf_ssl_context_t - pointer
 * [1] - data to write - buffer or string
 */
static duk_ret_t js_ssl_write(duk_context *ctx) {
	char errortext[256];
	LOGD(">> js_ssl_write");
	dukf_ssl_context_t *dukf_ssl_context = duk_get_pointer(ctx, -2);
	size_t len;
	uint8_t *buf;

	// If the data is a string, then the string is the data to transmit else it
	// is a buffer and the content of the buffer is the data to transmit,
	if (duk_is_string(ctx, -1)) {
		buf = (void *)duk_get_string(ctx, -1);
		len = strlen((char *)buf);
	} else {
		buf = duk_get_buffer_data(ctx, -1, &len);
		if (len == 0) {
			LOGE("js_ssl_write: The data buffer is zero length.");
			return 0;
		}
	}

	LOGD("About to send data over SSL: %.*s", len, buf);
	int rc = mbedtls_ssl_write(&dukf_ssl_context->ssl, buf, len);
	if (rc < 0) {
		 mbedtls_strerror(rc, errortext, sizeof(errortext));
		 LOGE("error from mbedtls_ssl_write: %d - %x - %s", rc, rc, errortext);
		 duk_push_nan(ctx);
	} else {
		duk_push_int(ctx, rc);
	}
	LOGD("<< js_ssl_write: rc=%d", rc);
	return 1;
} // js_ssl_write


/**
 * Add native methods to the SSL object.
 * [0] - SSL Object
 */
duk_ret_t ModuleSSL(duk_context *ctx) {

	ADD_FUNCTION("create_dukf_ssl_context", js_ssl_create_dukf_ssl_context, 2);
	ADD_FUNCTION("debugThreshold",          js_ssl_debug_set_threshold,     1);
	ADD_FUNCTION("free_dukf_ssl_context",   js_ssl_free_dukf_ssl_context,   1);
	ADD_FUNCTION("read",                    js_ssl_read,                    2);
	ADD_FUNCTION("write",                   js_ssl_write,                   2);

	return 0;
} // ModuleSSL
