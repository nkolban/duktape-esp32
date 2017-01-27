/*
 * ADC module.
 */

/* globals ESP32, log, module */


var moduleADC = ESP32.getNativeFunction("ModuleADC");
if (moduleADC === null) {
	log("Unable to find ModuleADC");
	module.exports = null;
	return;
}

var internalADC = {};
moduleADC(internalADC);

var _ret = {

		getValue: function(channel) {
			return internalADC.get_voltage(channel);
		},
		
		setResolution: function(resolution) {
			internalADC.config_width(resolution);
		},
		
		setAttenuation: function(channel, attenuation) {
			internalADC.config_channel_atten(channel, attenuation);
		},
		
		WIDTH_9BIT: internalADC.ADC_WIDTH_9BIT,
		WIDTH_10BIT: internalADC.ADC_WIDTH_10BIT,
		WIDTH_11BIT: internalADC.ADC_WIDTH_11BIT,
		WIDTH_12BIT: internalADC.ADC_WIDTH_12BIT,
		ATTEN_0DB: internalADC.ADC_ATTEN_0DB,
		ATTEN_2_5DB: internalADC.ADC_ATTEN_2_5DB,		
		ATTEN_6DB: internalADC.ADC_ATTEN_6DB,		
		ATTEN_11DB: internalADC.ADC_ATTEN_11DB,	
		CHANNEL_0: internalADC.ADC1_CHANNEL_0,
		CHANNEL_1: internalADC.ADC1_CHANNEL_1,
		CHANNEL_2: internalADC.ADC1_CHANNEL_2,
		CHANNEL_3: internalADC.ADC1_CHANNEL_3,
		CHANNEL_4: internalADC.ADC1_CHANNEL_4,
		CHANNEL_5: internalADC.ADC1_CHANNEL_5,
		CHANNEL_6: internalADC.ADC1_CHANNEL_6,
		CHANNEL_7: internalADC.ADC1_CHANNEL_7
}; // ret

module.exports = _ret;