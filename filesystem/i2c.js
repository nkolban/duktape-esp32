/*
 * I2C module.
 * 
 * configureMaster:
 * o port num
 * o sda_pin
 * o scl_pin
 * o clk_speed
 */

/* globals ESP32, log, module, require */


var moduleI2C = ESP32.getNativeFunction("ModuleI2C");
if (moduleI2C === null) {
	log("Unable to find ModuleI2C");
	module.exports = null;
	return;
}

var internalI2C = {};
moduleI2C(internalI2C);

var i2c_func = function(options) {
	log("options: " + JSON.stringify(options));
	log("internalI2C = " + internalI2C);
	log("internalI2C.param_config = " + internalI2C.param_config);
	internalI2C.param_config(options);
	internalI2C.driver_install(options);
	var cmd;
	
	return {
		//
		// beginTransaction
		//
		beginTransaction: function(address) {
			cmd = internalI2C.cmd_link_create();
			internalI2C.master_start(cmd);
			internalI2C.master_write_byte(cmd, (address << 1) | internalI2C.I2C_MASTER_WRITE, true);
		}, // beginTransaction
		
		//
		// endTransaction
		//
		endTransaction: function() {
			internalI2C.master_stop(cmd);
			var rc = internalI2C.master_cmd_begin(options.port, cmd, 1000);
			internalI2C.cmd_link_delete(cmd);
			cmd = null;
			return rc;
		}, // endTransaction
		
		//
		// write
		//
		write: function(data) {
			
		} // write
	}; // return
}; // i2c_func

i2c_func.I2C_MODE_MASTER  = internalI2C.I2C_MODE_MASTER;
i2c_func.I2C_MODE_SLAVE   = internalI2C.I2C_MODE_SLAVE;
i2c_func.I2C_NUM_0        = internalI2C.I2C_NUM_0;
i2c_func.I2C_NUM_1        = internalI2C.I2C_NUM_1;
i2c_func.I2C_MASTER_READ  = internalI2C.I2C_MASTER_READ;
i2c_func.I2C_MASTER_WRITE = internalI2C.I2C_MASTER_WRITE;

module.exports = i2c_func;