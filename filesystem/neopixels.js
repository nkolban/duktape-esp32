/* globals require, module */

function ws2812(gpioNum, pixelCount, channel) {
	if (channel === undefined) {
		channel = 0;
	}
	var RMT = require("RMT");
	
	// Configure the RMT channel.  A clock divider of 8 gives us a granularity of
	// 80MHz / 8 = 100ns per tick.
	RMT.txConfig(channel, {
		gpio: gpioNum,
		clockDiv: 8,
		memBlocks: 6
	});
	
	// Storage for the pixel values.  Each pixel will be an integer 24 bits in length
	// 8 bits for red, 8 bits for green and 8 bits for blue.
	var pixelData = new Array(pixelCount);
	
	// Initialize the RMT ...
	var ret = {
		//
		// clear the pixels by setting their values to 0.
		//
		clear: function() {
			var i;
			for (i=0; i<pixelCount; i++) {
				pixelData[i] = 0;
			}
		}, // clear
		
		//
		// show
		//
		show: function() {
			var i;
			// The RMT data is an array of items to be passed to the RMT controller.
			// Each item contains a level and a duration.  The level is the logic level
			// that will manifest on the output and duration is the length of the signal in
			// clock ticks where each clock tick is 100ns.  Since we want to drive the
			// neopixel data where each pixel is 24 bits of data and there is a high period
			// and a low period (i.e. 2 signals) then we need to allocate the number of items
			// as:
			// number of pixels * 2 signal changes per pixel * 24 bits of data per pixel
			//
			var rmtData = new Array(pixelCount * 24 * 2);
			var rmtIndex = 0;
			
			function setItem1() {
				rmtData[rmtIndex] = {
					level: true,
					duration: 8
				};
				rmtIndex++;
				rmtData[rmtIndex] = {
					level: false,
					duration: 6
				};
				rmtIndex++;
			} // setItem0
			
			function setItem0() {
				rmtData[rmtIndex] = {
					level: true,
					duration: 4
				};
				rmtIndex++;
				rmtData[rmtIndex] = {
					level: false,
					duration: 7
				};
				rmtIndex++;
			} // setItem1
			
			// Iterate over each pixel ...
			for (i=0; i<pixelCount; i++) {
				// prepare pixelData[i] for RMT
				var currentPixel = pixelData[i];
				for (var j=23; j>=0; j--) {
					if ((currentPixel & (1<<j)) !== 0) {
						setItem1();
					} else {
						setItem0();
					}
				} // For each bit in pixel
			} // for each pixel
			// Push to RMT
			//log("channel = " + channel);
			//log("rmtData = " + JSON.stringify(rmtData));
			//log("RMT = " + RMT);
			RMT.write(channel, rmtData);
			rmtData = null;

		}, // show
		
		//
		// setPixel
		//
		setPixel: function(pixel, red, green, blue) {
			if (pixel < 0 || pixel >= pixelCount) {
				return;
			}
			pixelData[pixel] = (red & 0xff) << 8 | (green & 0xff) << 16 | ((blue &0xff));
		} // setPixel
	}; // end of ret
	ret.clear();
	return ret;
} // End of ws2812

module.exports = ws2812;