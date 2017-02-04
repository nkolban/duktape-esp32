var LEDC = require("ledc");
var bitSize = 15;
var minValue = 500;  // micro seconds (uS)
var maxValue = 2500; // micro seconds (uS)
var sweepDuration = 1500; // milliseconds (ms)
var duty = (1<<bitSize) * minValue / 20000 ;
var direction = 1; // 1 = up, -1 = down
var valueChangeRate = 20; // msecs

LEDC.configureTimer({
	bitSize: bitSize,			
	freq: 50,
	timer: 0
});

LEDC.configureChannel({
	channel: 0,
	duty: duty,
	gpio: 16,			
	timer: 0
});

var changesPerSweep = sweepDuration / valueChangeRate;
var changeDelta = (maxValue-minValue) / changesPerSweep;

log("sweepDuration: " + sweepDuration + " seconds");
log("changesPerSweep: " + changesPerSweep);
log("changeDelta: " + changeDelta);
log("valueChangeRate: " + valueChangeRate);

var counter = 0;

setInterval(function() {
	if (counter >= changesPerSweep) {
		direction = -direction;
		counter = 0;
		log("Direction now " + direction);
	}
	if (direction > 0) {
		duty += changeDelta;
	} else {
		duty -= changeDelta;
	}
	LEDC.setDuty(0, duty);
	counter++;
}, valueChangeRate);