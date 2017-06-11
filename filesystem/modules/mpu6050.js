/*
 * MPU-6050 accelerometer
 * The MPU-6050 is an I2C accelerometer.
 */
const I2C = require("i2c");
const i2cDevice = new I2C({
   sda_pin: 25,
   scl_pin: 26
});

const MPU6050_ACCEL_XOUT_H = 0x3B;
const MPU6050_GYRO_XOUT_H = 0x43;
const MPU6050_PWR_MGMT_1 = 0x6B;
const ADDRESS = 0x68;

i2cDevice.beginTransaction(ADDRESS); // Write mode
i2cDevice.endTransaction();



i2cDevice.beginTransaction(ADDRESS); // Write mode
i2cDevice.write(MPU6050_PWR_MGMT_1);
i2cDevice.write(0);
i2cDevice.endTransaction();



const mod = {
	getAccel: function() {
		i2cDevice.beginTransaction(ADDRESS);
		i2cDevice.write(MPU6050_ACCEL_XOUT_H); // Write
		i2cDevice.endTransaction();
		i2cDevice.beginTransaction(ADDRESS, false); // Read
		var data5 = new Buffer(5);
		var data1 = new Buffer(1);
		i2cDevice.read(data5);
		i2cDevice.read(data1, false);
		i2cDevice.endTransaction();
		var data = Buffer.concat([data5, data1], 6);
		var x = data.readInt16BE(0);
		var y = data.readInt16BE(2);
		var z = data.readInt16BE(4);
		return {
			x: x,
			y: y,
			z: z
		}
	}, // getAccel
	
	getGyro: function() {
		i2cDevice.beginTransaction(ADDRESS);
		i2cDevice.write(MPU6050_GYRO_XOUT_H); // Write
		i2cDevice.endTransaction();
		i2cDevice.beginTransaction(ADDRESS, false); // Read
		//var data = new Buffer(6);
		var data5 = new Buffer(5);
		var data1 = new Buffer(1);
		i2cDevice.read(data5);
		i2cDevice.read(data1, false);
		i2cDevice.endTransaction();
		var data = Buffer.concat([data5, data1], 6);
		var x = data.readInt16BE(0);
		var y = data.readInt16BE(2);
		var z = data.readInt16BE(4);
		return {
			x: x,
			y: y,
			z: z
		}
	}, // getGyro
};
module.exports = mod;