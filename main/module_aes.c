#include "sdkconfig.h"
//#if defined(CONFIG_AES_ENABLED)

#include <freertos/FreeRTOSConfig.h>
#include <duktape.h>
#include <esp_log.h>
#include <stdio.h>
#include <string.h>

#include "duktape_utils.h"
#include "logging.h"
#include "module_aes.h"
#include "crypto/includes.h"
#include "crypto/common.h"
#include "crypto/aes.h"
#include "mbedtls/aes.h"

LOG_TAG("module_aes");

static duk_ret_t js_aes_init(duk_context *ctx) {
	LOGD(">> init_aes");
//	int errRc;
	LOGD("<< init_aes");
	return 0;
} // init_aes

/**
 * fast_aes_128_cbc_encrypt - AES-128 CBC encryption
 * @key: Encryption key
 * @iv: Encryption IV for CBC mode (16 bytes)
 * @data: Data to encrypt in-place
 * @data_len: Length of data in bytes (must be divisible by 16)
 * Returns: 0 on success, -1 on failure
  fast_aes_128_cbc_encrypt(const uint8_t *key, const uint8_t *iv, uint8_t *data, size_t data_len)
 */
static duk_ret_t js_fast_aes_128_cbc_encrypt(duk_context *ctx) {
// [0] - key <Buffer>
// [1] - iv <Buffer>
// [2] - data <Buffer>
// [3] - data_len <Integer>

	duk_size_t keySize;
	uint8_t *key = duk_require_buffer_data(ctx, 0, &keySize);

	duk_size_t ivSize;
	uint8_t *iv = duk_require_buffer_data(ctx, 1, &ivSize);

	duk_size_t dataSize;
	uint8_t *data = duk_require_buffer_data(ctx, 2, &dataSize);

	int data_len = duk_require_int(ctx, 3);

	int ret = 0;
	mbedtls_aes_context aes_ctx;
	uint8_t cbc[AES_BLOCK_SIZE];

	mbedtls_aes_init(&aes_ctx);

	ret = mbedtls_aes_setkey_enc(&aes_ctx, key, 128);

	if(ret < 0) {
		mbedtls_aes_free(&aes_ctx);
		return 0; // FIX
	}

	os_memcpy(cbc, iv, AES_BLOCK_SIZE);

	ret = mbedtls_aes_crypt_cbc(&aes_ctx, MBEDTLS_AES_ENCRYPT, data_len, cbc, data, data);

	mbedtls_aes_free(&aes_ctx);

	return 0;
}


/**
 * fast_aes_128_cbc_decrypt - AES-128 CBC decryption
 * @key: Decryption key
 * @iv: Decryption IV for CBC mode (16 bytes)
 * @data: Data to decrypt in-place
 * @data_len: Length of data in bytes (must be divisible by 16)
 * Returns: 0 on success, -1 on failure
  fast_aes_128_cbc_decrypt(const uint8_t *key, const uint8_t *iv, uint8_t *data, size_t data_len)
 */
static duk_ret_t js_fast_aes_128_cbc_decrypt(duk_context *ctx) {
// [0] - key <Buffer>
// [1] - iv <Buffer>
// [2] - data <Buffer>
// [3] - data_len <Integer>

	duk_size_t keySize;
	uint8_t *key = duk_require_buffer_data(ctx, 0, &keySize);

	duk_size_t ivSize;
	uint8_t *iv = duk_require_buffer_data(ctx, 1, &ivSize);

	duk_size_t dataSize;
	uint8_t *data = duk_require_buffer_data(ctx, 2, &dataSize);

	int data_len = duk_require_int(ctx, 3);

	int ret = 0;
	mbedtls_aes_context aes_ctx;
	uint8_t cbc[AES_BLOCK_SIZE];

	mbedtls_aes_init(&aes_ctx);

	ret = mbedtls_aes_setkey_dec(&aes_ctx, key, 128);

	if(ret < 0) {
		mbedtls_aes_free(&aes_ctx);
		return 0; // FIX
	}

	os_memcpy(cbc, iv, AES_BLOCK_SIZE);

	ret = mbedtls_aes_crypt_cbc(&aes_ctx, MBEDTLS_AES_DECRYPT, data_len, cbc, data, data);

	mbedtls_aes_free(&aes_ctx);

	return 0;
}


/**
 * fast_aes_wrap - Wrap keys with AES Key Wrap Algorithm (128-bit KEK) (RFC3394)
 * @kek: 16-octet Key encryption key (KEK)
 * @n: Length of the plaintext key in 64-bit units; e.g., 2 = 128-bit = 16
 * bytes
 * @plain: Plaintext key to be wrapped, n * 64 bits
 * @cipher: Wrapped key, (n + 1) * 64 bits
 * Returns: 0 on success, -1 on failure
   fast_aes_wrap(const uint8_t *kek, int n, const uint8_t *plain, uint8_t *cipher)
 */
static duk_ret_t js_fast_aes_wrap(duk_context *ctx) {
// [0] - kek <Buffer>
// [1] - n <Integer>
// [2] - cipher <Buffer>
// [3] - plain <Buffer>

	duk_size_t kekSize;
	uint8_t *kek = duk_require_buffer_data(ctx, 0, &kekSize);

	int n = duk_require_int(ctx, 1);

	duk_size_t cipherSize;
	uint8_t *cipher = duk_require_buffer_data(ctx, 2, &cipherSize);

	duk_size_t plainSize;
	uint8_t *plain = duk_require_buffer_data(ctx, 2, &plainSize);

	uint8_t *a, *r, b[16];
	int32_t i, j;
	int32_t ret = 0;
	mbedtls_aes_context aes_ctx;

	a = cipher;
	r = cipher + 8;

	/* 1) Initialize variables. */
	os_memset(a, 0xa6, 8);
	os_memcpy(r, plain, 8 * n);

	mbedtls_aes_init(&aes_ctx);
	ret = mbedtls_aes_setkey_enc(&aes_ctx, kek, 128);
	if (ret < 0) {
		mbedtls_aes_free(&aes_ctx);
		return 0; // FIX
	}

	/* 2) Calculate intermediate values.
	 * For j = 0 to 5
	 *	 For i=1 to n
	 *		 B = AES(K, A | R[i])
	 *		 A = MSB(64, B) ^ t where t = (n*j)+i
	 *		 R[i] = LSB(64, B)
	 */
	for (j = 0; j <= 5; j++) {
	r = cipher + 8;
	for (i = 1; i <= n; i++) {
			os_memcpy(b, a, 8);
			os_memcpy(b + 8, r, 8);
			mbedtls_aes_encrypt(&aes_ctx, b, b);
			os_memcpy(a, b, 8);
			a[7] ^= n * j + i;
			os_memcpy(r, b + 8, 8);
			r += 8;
	}
	}
	mbedtls_aes_free(&aes_ctx);

	return 0;
}

/**
 * fast_aes_unwrap - Unwrap key with AES Key Wrap Algorithm (128-bit KEK) (RFC3394)
 * @kek: Key encryption key (KEK)
 * @n: Length of the plaintext key in 64-bit units; e.g., 2 = 128-bit = 16
 * bytes
 * @cipher: Wrapped key to be unwrapped, (n + 1) * 64 bits
 * @plain: Plaintext key, n * 64 bits
 * Returns: 0 on success, -1 on failure (e.g., integrity verification failed)
   fast_aes_unwrap(const uint8_t *kek, int n, const uint8_t *cipher, uint8_t *plain)
 */
static duk_ret_t js_fast_aes_unwrap(duk_context *ctx) {
// [0] - kek <Buffer>
// [1] - n <Integer>
// [2] - cipher <Buffer>
// [3] - plain <Buffer>

	duk_size_t kekSize;
	uint8_t *kek = duk_require_buffer_data(ctx, 0, &kekSize);

	int n = duk_require_int(ctx, 1);

	duk_size_t cipherSize;
	uint8_t *cipher = duk_require_buffer_data(ctx, 2, &cipherSize);

	duk_size_t plainSize;
	uint8_t *plain = duk_require_buffer_data(ctx, 2, &plainSize);

	uint8_t a[8], *r, b[16];
	int32_t i, j;
	int32_t ret = 0;
	mbedtls_aes_context aes_ctx;

	/* 1) Initialize variables. */
	os_memcpy(a, cipher, 8);
	r = plain;
	os_memcpy(r, cipher + 8, 8 * n);

	mbedtls_aes_init(&aes_ctx);
	ret = mbedtls_aes_setkey_dec(&aes_ctx, kek, 128);
	if (ret < 0) {
		mbedtls_aes_free(&aes_ctx);
		return 0; // FIX!!!!
	}

	/* 2) Compute intermediate values.
	 * For j = 5 to 0
	 *	 For i = n to 1
	 *		 B = AES-1(K, (A ^ t) | R[i]) where t = n*j+i
	 *		 A = MSB(64, B)
	 *		 R[i] = LSB(64, B)
	 */
	for (j = 5; j >= 0; j--) {
		r = plain + (n - 1) * 8;
		for (i = n; i >= 1; i--) {
			os_memcpy(b, a, 8);
			b[7] ^= n * j + i;
			os_memcpy(b + 8, r, 8);
			mbedtls_aes_decrypt(&aes_ctx, b, b);
			os_memcpy(a, b, 8);
			os_memcpy(r, b + 8, 8);
			r -= 8;
		}
	}
	mbedtls_aes_free(&aes_ctx);

	return 0;
}

/**
 * Add native methods to the AES object.
 * [0] - AES Object
 */
duk_ret_t ModuleAES(duk_context *ctx) {
	ADD_FUNCTION("init", js_aes_init, 0);
	ADD_FUNCTION("fast_aes_128_cbc_encrypt", js_fast_aes_128_cbc_encrypt, 4);
	ADD_FUNCTION("fast_aes_128_cbc_decrypt", js_fast_aes_128_cbc_decrypt, 4);
	ADD_FUNCTION("fast_aes_wrap",   js_fast_aes_wrap, 4);
	ADD_FUNCTION("fast_aes_unwrap", js_fast_aes_unwrap, 4);
	return 0;
} // ModuleAES
//#endif

