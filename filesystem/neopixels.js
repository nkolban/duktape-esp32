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
			// var rmtData = new Array(pixelCount * 24 * 2);
			//
			// If we have "pixelCount" pixels, how many bytes do we need for an RMT buffer?
			// Here is how we calculate this:
			// First there are 24 bits per pixel.  In NeoPixels, we need a high period and 
			// a low period for each bit.  Translating this to RMT data items, we need
			// TWO data items per bit which means we need:
			//
			// pixelCount * 24 * 2 RMT data items.  As order of magnitude examples:
			//
			// 1 pixel    = 48 RMT data items
			// 10 pixels  = 480 RMT data items
			// 100 pixels = 4800 RMT data items
			//
			// Since an RMT data item is 16 bits (1 bit for level and 15 bits for duration)
			// and 16 bits is 2 bytes ... the amount o bytes needed for our RMT data then becomes:
			//
			// pixelCount * 24 * 2 * 2 bytes.  As order of magnitude examples:
			//
			// 1 pixel    = 96 bytes
			// 10 pixels  = 960 bytes
			// 100 pixels = 9600 bytes
			//
			var rmtData = new Buffer(pixelCount * 24 * 2 * 2);
			var offset = 0;
			
			function writeDataItem(level, duration) {
				var value;
				if (level) {
					value = 0x8000;
				} else {
					value = 0x0000;
				}
				value = (duration & 0x7fff) | value;
				rmtData.writeUInt16LE(value, offset);
				offset += 2;
			} // writeDataItem
			
			function setItem1() {
				writeDataItem(true, 8);
				writeDataItem(false, 6);
			} // setItem0
			
			function setItem0() {
				writeDataItem(true, 4);
				writeDataItem(false, 7);
			} // setItem1
			
			// Iterate over each pixel ...
			for (i=0; i<pixelCount; i++) {
				// prepare pixelData[i] for RMT
				//log("Setting pixel: " + i + ": free heap = " + ESP32.getState().heapSize);
				var currentPixel = pixelData[i];
				for (var j=23; j>=0; j--) {
					if ((currentPixel & (1<<j)) !== 0) {
						//setItem1();
						writeDataItem(true, 8);
						writeDataItem(false, 6);
					} else {
						//setItem0();
						writeDataItem(true, 4);
						writeDataItem(false, 7);
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
		// Set the value of the pixel to the supplied red/green/blue
		// components.  The signature of the function can be:
		//
		// setPixel(pixel, red, green, blue)
		// setPixel(pixel, "#rrggbb")
		//
		setPixel: function(pixel, red, green, blue) {
			if (typeof(red) == "number") {
				
			} else if (typeof(red) == "string") {
				sValue = red;
				// #RRGGBB
				// 0123456
				if (sValue.length != 7 || sValue[0] != '#') {
					log("Unknown string value passed to setPixel");

					return;
				}
				red   = parseInt(sValue.substring(1, 3), 16);
				green = parseInt(sValue.substring(3, 5), 16);
				blue  = parseInt(sValue.substring(5, 7), 16);
			} else {
				log("Unknown value passed to setPixel");
				return;
			}
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