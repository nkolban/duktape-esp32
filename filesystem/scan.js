WIFI.scan(function(data){
	log("scan data: " + JSON.stringify(data));
});
log("callback stash: " + JSON.stringify(callbackStash));