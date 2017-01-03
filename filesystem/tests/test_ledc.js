var LEDC = require("ledc");
log("Configuring timer ...");
LEDC.configureTimer({
	bitSize: 12,
	freq: 1000,
	timer: 0
});

log("Configuring channel ...");
LEDC.configureChannel({
	channel:0,
	duty: 4000,	
	gpio: 22,
	timer: 0
});

log("LEDC configured!");