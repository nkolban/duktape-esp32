var wifi = require("WiFi");
wifi.scan(function(list){
	console.log(JSON.stringify(list));
});