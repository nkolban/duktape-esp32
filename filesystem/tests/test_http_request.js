var http = require("http.js");
var net = require("net.js");

function http_test1() {
	var address = "www.httbin.org"; // httpbin.org
	log("Address: " + address);
	if (address === null) {
		return;
	}
	function logHTTP(obj) {
		log("HTTP Status Code: " + obj.httpStatus);
		log("Headers: " + JSON.stringify(obj.headers));
	}

	http.request({
		address : address,
		port : 80,
		path : "/get"
	}, function(response) {
		log("HTTP Response ready ...");
		response.on("data", function(data) {
			log("**************")
			log("Response data:");
			logHTTP(response);
			log(data);
			log("**************")

		});
		response.on("end", function() {
			log("**********************")
			log("*** Response complete");
			logHTTP(response);
			log("**********************")
		})
	});
}

http_test1();
