/*
SHA1("The quick brown fox jumps over the lazy dog")
gives hexadecimal: 2fd4e1c67a2d28fced849ee1bb76e7391b93eb12
gives Base64 binary to ASCII text encoding: L9ThxnotKPzthJ7hu3bnORuT6xI=
*/
var data = "The quick brown fox jumps over the lazy dog";
var result = OS.sha1(data);
log(Duktape.enc('base64', result));
if (Duktape.enc('base64', result) == "L9ThxnotKPzthJ7hu3bnORuT6xI=") {
	log("Succesfully encoded");
} else {
	log("Failed to encode");
}