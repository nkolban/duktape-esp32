var mpu6050 = require("modules/mpu6050");
setInterval(function () {
	var accel = mpu6050.getAccel();
	var gyro = mpu6050.getGyro();
	log("Accel: " + JSON.stringify(accel) + ", Gyro: " + JSON.stringify(gyro));
}, 1000);