var I2C = require("i2c");
log("Constants:");
log("I2C_NUM_0: " + I2C.I2C_NUM_0);
log("I2C_NUM_1: " + I2C.I2C_NUM_1);
log("I2C_MODE_MASTER: " + I2C.I2C_MODE_MASTER);
log("I2C_MODE_SLAVE: " + I2C.I2C_MODE_SLAVE);
log("I2C_MASTER_READ: " + I2C.I2C_MASTER_READ);
log("I2C_MASTER_WRITE: " + I2C.I2C_MASTER_WRITE);

var port1 = new I2C({
	port: I2C.I2C_NUM_0,
	mode: I2C.I2C_MODE_MASTER,
	sda_pin: 21,
	scl_pin: 22,
	master_clk_speed: 100000
});

var i;

for (i=3; i<0x78; i++) {
	port1.beginTransaction(i);
	var rc = port1.endTransaction();
	if (rc == 0) {
		log("*** found one: " + i);
	}
}

/*
i=10;
port1.beginTransmission(i);
port1.endTransmission();
*/

/*
I2C.param_config({
	port: I2C.I2C_NUM_0,
	mode: I2C.I2C_MODE_MASTER,
	sda_pin: 25,
	scl_pin: 26,
	master_clk_speed: 100000
});

I2C.driver_install({
	port_num: I2C.I2C_NUM_0,
	mode: I2C.I2C_MODE_MASTER
});
*/