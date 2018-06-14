# AES module
Use the Uint8Array.allocPlain() for buffer allocation.
cbc_encrypt and cbc_decrypt complementary functions.

## Example
var aes = require('aes');
var alloc8 = Uint8Array.allocPlain;
function ta(base, len) {
	var arr = alloc8(len);
	for(var i=0;i<len;++i) arr[i]=base+i;
	return arr;
}
var key = ta(0, 16);
var iv = ta(16, 16);
var temp = ta(64, 32);
//hexprint('1 ', temp);
aes.fast_aes_128_cbc_encrypt(key, iv, temp, temp.length);
//hexprint('2 ', temp);
aes.fast_aes_128_cbc_decrypt(key, iv, temp, temp.length);
//hexprint('3 ', temp);
