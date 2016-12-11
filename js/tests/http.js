var http = require("http");

function http_test1() {
	var address = "54.175.219.8"; // httpbin.org
	var clientRequest = http.request({address: address, port: 80, path: "/get"}, function(response) {
		log("HTTP Response ready ...");
	});
}

http_test1();
