function ws2812(channel, gpioNum, pixelCount) {
	RMT.txConfig(channel, {
		gpio: gpioNum,
		clockDiv: 8
	});
	var pixelData = new Array(pixelCount); // Storage for the pixel values
	// Initialize the RMT ...
	var ret = {
		//
		// clear
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
			var rmtData = new Array(pixelCount * 24 * 2);
			rmtIndex = 0;
			function setItem1() {
				rmData[rmtIndex] = {
					level: true,
					duration: 7
				}
				rmtIndex++;
				rmData[rmtIndex] = {
					level: false,
					duration: 6
				}
				rmtIndex++;
			}
			function setItem2() {
				rmData[rmtIndex] = {
					level: true,
					duration: 4
				}
				rmtIndex++;
				rmData[rmtIndex] = {
					level: false,
					duration: 8
				}
				rmtIndex++;
			}
			for (i=0; i<pixelCount; i++) {
				// prepare pixelData[i] for RMT
				var currentPixel = pixelData[i];
				for (var j=0; j<24; j++) {
					if (currentPixel & (1<<j) != 0) {
						setItem1();
					} else {
						setItem0();
					}
				} // For each bit in pixel
			} // for each pixel
			// Push to RMT
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
			pixelData[pixel] = red | (green << 8) | (blue << 16);
		} // setPixel
	}
	ret.clear();
	return ret;
}
module.exports = ws2812;