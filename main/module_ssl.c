#include <mbedtls/platform.h> // Has to come early

#include <mbedtls/ctr_drbg.h>
#include <mbedtls/debug.h>
#include <mbedtls/entropy.h>
#include <mbedtls/error.h>
#include <mbedtls/net.h>
#include <mbedtls/ssl.h>
#include <string.h>

#include "logging.h"
#include "module_ssl.h"

LOG_TAG("ssl");

/*
 * Provide support for SSL (secure sockets layer) from within the ESP32-Duktape environment.
 */
typedef struct {
  mbedtls_x509_crt cacert;
  mbedtls_ssl_config conf;
  mbedtls_ctr_drbg_context ctr_drbg;
  mbedtls_entropy_context entropy;
  mbedtls_net_context server_fd;
  mbedtls_ssl_context ssl;
} ssl_socket_t;

const static char *pers = "ssl_client1";

static ssl_socket_t *create_ssl_socket() {
	char errortext[256];
	ssl_socket_t *ssl_socket;

	ssl_socket = malloc(sizeof(ssl_socket_t));
	mbedtls_net_init(&ssl_socket->server_fd);
	mbedtls_ssl_init(&ssl_socket->ssl);
	mbedtls_ssl_config_init(&ssl_socket->conf);
	mbedtls_x509_crt_init(&ssl_socket->cacert);
	mbedtls_ctr_drbg_init(&ssl_socket->ctr_drbg);
	mbedtls_entropy_init(&ssl_socket->entropy);

  int rc = mbedtls_ctr_drbg_seed(
     &ssl_socket->ctr_drbg,
     mbedtls_entropy_func, &ssl_socket->entropy, (const unsigned char *) pers, strlen(pers));
  if (rc != 0) {
     LOGE(" failed\n  ! mbedtls_ctr_drbg_seed returned %d", rc);
     return NULL;
  }

  rc = mbedtls_ssl_config_defaults(
		&ssl_socket->conf,
		MBEDTLS_SSL_IS_CLIENT,
		MBEDTLS_SSL_TRANSPORT_STREAM,
		MBEDTLS_SSL_PRESET_DEFAULT);
	if (rc != 0) {
		 LOGE("mbedtls_ssl_config_defaults returned %d", rc);
		 return NULL;
	}

	mbedtls_ssl_conf_authmode(&ssl_socket->conf, MBEDTLS_SSL_VERIFY_NONE);

	mbedtls_ssl_conf_rng(&ssl_socket->conf, mbedtls_ctr_drbg_random, &ssl_socket->ctr_drbg);

	rc = mbedtls_ssl_setup(&ssl_socket->ssl, &ssl_socket->conf);
	if (rc != 0) {
		 mbedtls_strerror(rc, errortext, sizeof(errortext));
		 LOGE("error from mbedtls_ssl_setup: %d - %x - %s\n", rc, rc, errortext);
		 return NULL;
	}

	rc = mbedtls_ssl_set_hostname(&ssl_socket->ssl, "httpbin.org");
	if (rc) {
		 mbedtls_strerror(rc, errortext, sizeof(errortext));
		 LOGE("error from mbedtls_ssl_set_hostname: %d - %x - %s", rc, rc, errortext);
		 return NULL;
	}
	return ssl_socket;
} // create_ssl_socket


static int connect_ssl_socket(ssl_socket_t *ssl_socket, const char *hostname, uint16_t port) {
	if (ssl_socket == NULL) {
		LOGE("ssl_socket was NULL");
		return -1;
	}
	if (hostname == NULL) {
		LOGE("hostname was NULL");
		return -1;
	}
	char port_string[10];
	char errortext[256];
	sprintf(port_string, "%d", port);
	int rc = mbedtls_net_connect(&ssl_socket->server_fd, hostname, port_string, MBEDTLS_NET_PROTO_TCP);
	if (rc != 0) {
		LOGE("mbedtls_net_connect returned %d", rc);
		mbedtls_strerror(rc, errortext, sizeof(errortext));
		LOGE("error from mbedtls_net_connect: %d - %x - %s", rc, rc, errortext);
		return rc;
	}
	return 0;
} // connect_ssl_socket


static void free_ssl_socket(ssl_socket_t *ssl_socket) {
  mbedtls_net_free(&ssl_socket->server_fd);
  mbedtls_ssl_free(&ssl_socket->ssl);
  mbedtls_ssl_config_free(&ssl_socket->conf);
  mbedtls_ctr_drbg_free(&ssl_socket->ctr_drbg);
  mbedtls_entropy_free(&ssl_socket->entropy);
  free(ssl_socket);
} // free_ssl_socket

/*
 * [0] - ptr to ssl_socket
 * [1] - hostname
 * [2] - port
 */
static duk_ret_t js_ssl_connect(duk_context *ctx) {
	LOGD(">> js_ssl_connect");
	ssl_socket_t *ssl_socket = duk_get_pointer(ctx, -3);
	const char *hostname = duk_get_string(ctx, -2);
	int port = duk_get_int(ctx, -1);
	LOGD(" - Hostname: %s, port: %d", hostname, port);
	connect_ssl_socket(ssl_socket, hostname, (uint16_t)port);
	LOGD("<< js_ssl_connect");
	return 0;
}


static duk_ret_t js_ssl_createSSLSocket(duk_context *ctx) {
	LOGD(">> js_ssl_createSSLSocket");
	ssl_socket_t *ssl_socket = create_ssl_socket();
	duk_push_pointer(ctx, ssl_socket);
	LOGD("<< js_ssl_createSSLSocket");
	return 1;
} // js_ssl_createSSLSocket


static duk_ret_t js_ssl_test(duk_context *ctx) {
	ssl_socket_t *ssl_socket = create_ssl_socket();
	if (ssl_socket == NULL) {
		return 0;
	}
	int rc = connect_ssl_socket(ssl_socket, "httpbin.org", 443);
	free_ssl_socket(ssl_socket);
	LOGD("SSL test complete! rc=%d", rc);
	return 0;
}


/**
 * Add native methods to the SSL object.
 * [0] - SSL Object
 */
duk_ret_t ModuleSSL(duk_context *ctx) {

	int idx = -2;
	duk_push_c_function(ctx, js_ssl_connect, 0);
	// [0] - SSL object
	// [1] - C Function - js_ssl_connect

	duk_put_prop_string(ctx, idx, "connect"); // Add connect to SSL
	// [0] - SSL object

	duk_push_c_function(ctx, js_ssl_createSSLSocket, 0);
	// [0] - SSL object
	// [1] - C Function - js_ssl_createSSLSocket

	duk_put_prop_string(ctx, idx, "createSSLSocket"); // Add createSSLSocket to SSL
	// [0] - SSL object

	duk_push_c_function(ctx, js_ssl_test, 2);
	// [0] - SSL object
	// [1] - C Function - js_ssl_test

	duk_put_prop_string(ctx, idx, "test"); // Add test to SSL
	// [0] - SSL object

	duk_pop(ctx);
	// <Empty Stack>
	return 0;
} // ModuleSSL
