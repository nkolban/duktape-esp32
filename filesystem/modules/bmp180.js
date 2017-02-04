/* globals require, Buffer, log, DUKF */

var I2C = require("i2c");

var BMP085_REGISTER_CAL_AC1            = 0xAA;  // R   Calibration data (16 bits)
var BMP085_REGISTER_CAL_AC2            = 0xAC;  // R   Calibration data (16 bits)
var BMP085_REGISTER_CAL_AC3            = 0xAE;  // R   Calibration data (16 bits)
var BMP085_REGISTER_CAL_AC4            = 0xB0;  // R   Calibration data (16 bits)
var BMP085_REGISTER_CAL_AC5            = 0xB2;  // R   Calibration data (16 bits)
var BMP085_REGISTER_CAL_AC6            = 0xB4;  // R   Calibration data (16 bits)
var BMP085_REGISTER_CAL_B1             = 0xB6;  // R   Calibration data (16 bits)
var BMP085_REGISTER_CAL_B2             = 0xB8;  // R   Calibration data (16 bits)
var BMP085_REGISTER_CAL_MB             = 0xBA;  // R   Calibration data (16 bits)
var BMP085_REGISTER_CAL_MC             = 0xBC;  // R   Calibration data (16 bits)
var BMP085_REGISTER_CAL_MD             = 0xBE;  // R   Calibration data (16 bits)

var BMP085_REGISTER_CHIPID             = 0xD0;
var BMP085_REGISTER_VERSION            = 0xD1;
var BMP085_REGISTER_SOFTRESET          = 0xE0;
var BMP085_REGISTER_CONTROL            = 0xF4;
var BMP085_REGISTER_TEMPDATA           = 0xF6;
var BMP085_REGISTER_PRESSUREDATA       = 0xF6;
var BMP085_REGISTER_READTEMPCMD        = 0x2E;
var BMP085_REGISTER_READPRESSURECMD    = 0x34; // 0011 0100
var BMP180_ADDRESS = 0x77;

var BMP085_MODE_ULTRALOWPOWER          = 0;
var BMP085_MODE_STANDARD               = 1;
var BMP085_MODE_HIGHRES                = 2;
var BMP085_MODE_ULTRAHIGHRES           = 3;

var bmp180 = new I2C({
	sda_pin: 18,
	scl_pin: 19
});

function centigrade_to_fahrenheit(centigrade) {
	return centigrade * 9 / 5 + 32.0;
}

function readRegister16(reg, unsignedFlag) {
	if (unsignedFlag === undefined) {
		unsignedFlag = false;
	}
	bmp180.beginTransaction(BMP180_ADDRESS);
	bmp180.write(reg);
	bmp180.endTransaction();
	
	bmp180.beginTransaction(BMP180_ADDRESS, false);
	var data = new Buffer(2);
	bmp180.read(data);
	bmp180.endTransaction();
	//log("data: [0]=" + data[0] + ", [1]=" + data[1]);

	var val;
	if (unsignedFlag) {
		val = data.readUInt16BE(0);
	} else {
		val = data.readInt16BE(0);
	}
	return val;
}

function readRegister24(reg) {
	bmp180.beginTransaction(BMP180_ADDRESS);
	bmp180.write(reg);
	bmp180.endTransaction();
	
	bmp180.beginTransaction(BMP180_ADDRESS, false);
	var data = new Buffer(3);
	bmp180.read(data);
	bmp180.endTransaction();
	//log("data: [0]=" + data[0] + ", [1]=" + data[1] + ", [2]=" + data[2]);

	var val = (data[0] << 16) | (data[1] << 8) | data[2];
	return val;
}



// Example from ESP32: 
// AC1=8959, AC2=-1025, AC3=17407, AC4=34047, AC5=255,
// AC6=17407, B1=6655, B2=255, MB=-32513, MC=255, MD=2815


// From C program:
//D (2667) bmp180: Params:
//AC1: 8959, AC2: -1025, AC3: -14337, AC4:34047, AC5: 25343,
// AC6: 17407, B1: 6655, B2: 255, MB: -32513, MC: -11777, MD: 2815

// From this we see deviations in AC6 and MD

var AC1;
var AC2;
var AC3;
var AC4;
var AC5;
var AC6;
var B1;
var B2;
var MB;
var MC;
var MD;

var haveCompensations = false;
function readCompensations() {
	haveCompensations = true;
	AC1 = readRegister16(BMP085_REGISTER_CAL_AC1);
	AC2 = readRegister16(BMP085_REGISTER_CAL_AC2);
	AC3 = readRegister16(BMP085_REGISTER_CAL_AC3);
	AC4 = readRegister16(BMP085_REGISTER_CAL_AC4, true); // unsigned
	AC5 = readRegister16(BMP085_REGISTER_CAL_AC5, true); // unsigned
	AC6 = readRegister16(BMP085_REGISTER_CAL_AC6, true); // unsigned
	B1  = readRegister16(BMP085_REGISTER_CAL_B1);
	B2  = readRegister16(BMP085_REGISTER_CAL_B2);
	MB  = readRegister16(BMP085_REGISTER_CAL_MB);
	MC  = readRegister16(BMP085_REGISTER_CAL_MC);
	MD  = readRegister16(BMP085_REGISTER_CAL_MD);
	//log ("AC1=" + AC1 + ", AC2=" + AC2 + ", AC3=" + AC3 + ", AC4=" + AC4 + ", AC5=" + AC5 + ", AC6=" + AC6 + ", B1=" + B1 + ", B2=" + B2 + ", MB=" + MB + ", MC=" + MC + ", MD=" + MD);
}


function readUncompensatedTemp() {
	bmp180.beginTransaction(BMP180_ADDRESS);
	bmp180.write(BMP085_REGISTER_CONTROL);
	bmp180.write(BMP085_REGISTER_READTEMPCMD);
	bmp180.endTransaction();	
	DUKF.sleep(5);
	return readRegister16(BMP085_REGISTER_TEMPDATA, true);
}

function readUncompensatedPressure(mode) {
	
	bmp180.beginTransaction(BMP180_ADDRESS);
	bmp180.write(BMP085_REGISTER_CONTROL);
	bmp180.write(BMP085_REGISTER_READPRESSURECMD + (mode << 6));
	bmp180.endTransaction();	
	switch(mode) {
	case BMP085_MODE_ULTRALOWPOWER:
		DUKF.sleep(5);
		break;
	case BMP085_MODE_STANDARD:
		DUKF.sleep(8);
		break;
	case BMP085_MODE_HIGHRES:
		DUKF.sleep(14);
		break;
	case BMP085_MODE_ULTRAHIGHRES:
		DUKF.sleep(26);
		break;
	}
	var ret;
	if (mode !== BMP085_MODE_ULTRAHIGHRES) {
		ret = readRegister24(BMP085_REGISTER_PRESSUREDATA);
	} else {
		ret = readRegister24(BMP085_REGISTER_PRESSUREDATA);
	}
	return ret >> (8-mode);
}

var mod = {
	getTemp: function() {
		if (!haveCompensations) {
			readCompensations();
		}
		var UT = readUncompensatedTemp();
		var X1 = ((UT - AC6) * AC5) >> 15;
		var X2 = (MC<<11) / (X1+MD);
		var B5 =  X1 + X2;
		var temp = ((B5 + 8)>>4)/10.0;
		return temp;
	},
	getPressure: function() {
		if (!haveCompensations) {
			readCompensations();
		}
		var bmpMode = BMP085_MODE_STANDARD;
		var UT = readUncompensatedTemp();
		var UP = readUncompensatedPressure(bmpMode); // Example from working C ... 87039
		var X1 = ((UT - AC6) * AC5) >> 15;
		var X2 = (MC<<11) / (X1+MD);
		var B5 =  X1 + X2;
		var B6 = B5 - 4000;
		X1 = (B2 * ((B6 * B6) >> 12)) >> 11;
		X2 = (AC2 * B6) >> 11;
		var X3 = X1 + X2;
		var B3 = (((AC1*4 + X3) << bmpMode) + 2) >> 2;
		X1 = (AC3 * B6) >> 13;
		X2 = (B1 * ((B6 * B6) >> 12)) >> 16;
		X3 = ((X1 + X2) + 2) >> 2;
		var B4 = (AC4 * (X3 + 32768)) >> 15;
		var B7 = (((UP) - B3) * (50000 >> bmpMode));
		var P;
		if (B7 < 0x80000000) {
			P = (B7 * 2) / B4;
		} else {
			P = (B7 / B4) * 2;
		}
		X1 = (P >> 8) * (P >> 8);
		X1 = (X1 * 3038) >> 16;
		X2 = (-7357 * P) >> 16;
		var pressure = P + ((X1 + X2 + 3791) >> 4);
		return Math.round(pressure);
	},
	c2f: function(centigrade) {
		return centigrade * 9 / 5 + 32.0;
	},
	pa2inHg: function(pressure) {
		return pressure * 0.00029530;
	}
};

module.exports = mod;