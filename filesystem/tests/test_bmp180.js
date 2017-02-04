var bmp180 = require("modules/bmp180");
setInterval(function() {
    var temp = bmp180.getTemp();
    var pressure = bmp180.getPressure();
    log("Temp: " + temp + "oC -> " + bmp180.c2f(temp) + "oF");
    log("Pressure: " + pressure + "pa -> " + Math.round(bmp180.pa2inHg(pressure) * 100)/100 + "inHg");
}, 2000);