var ADC = require("adc");
ADC.setResolution(ADC.WIDTH_12BIT);
setInterval(function() {
	log("Value: " + ADC.getValue(ADC.CHANNEL_0));
}, 1000);