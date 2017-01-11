/*
 * Test a MAX7219.
 */
ESP32.setLogLevel("*", "error");
var MOSI = 13;
var CLK  = 12;
var CS   = 14;

var SPI = require("spi");
var MAX7219 = require("modules/MAX7219");

SPI.initialize({
   mosi: MOSI,
   clk:  CLK
});

var spiDevice = SPI.addDevice({
	clock_speed: 10000,
	cs: CS
});

var disp = new MAX7219(spiDevice);

disp.setDecodeAll();
disp.setScanLimit(8);
disp.setDisplayIntensity(15);
disp.startup();

var counter = 0;
setInterval(function() {
	disp.setNumber(counter);
	counter++;
}, 10);
