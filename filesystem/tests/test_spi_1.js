/*
 * Test SPI.
 */
var SPI = require("spi");
SPI.initialize({
   mosi: 21,
   miso: -1,
   clk: 22
});
var device = SPI.addDevice({
	clock_speed: 10000
});

var data = Buffer(4);
data[0] = 0x12;
data[1] = 0x34;
data[2] = 0x56;
data[3] = 0x78;

device.transmit(data);


device.remove();
SPI.free();
