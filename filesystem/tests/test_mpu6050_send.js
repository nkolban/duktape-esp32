ESP32.setLogLevel("*", "none");
ESP32.setLogLevel("log", "verbose");
var g_sock = null;
var count = 0;

var mpu6050 = require("modules/mpu6050");
setInterval(function () {
	var accel = mpu6050.getAccel();
	//var gyro = mpu6050.getGyro();
	log("Accel: " + JSON.stringify(accel));
	if (g_sock != null) {
		g_sock.write(count + ": " + accel.x + "," + accel.y + "," + accel.z + "\n");
		count++;
	}
}, 100);

/*
 * In this sample we want to listen for incoming socket clients and when one attaches,
 * periodically send it data.  This will illustrate the technique of sending data.
 */

var net = require("net");
function createListener() {
	
	function connectionListener(sock) {
		g_sock = sock;
	   log("connectionListener: We have received a new connection!");
	   sock.on("data", function(data) {
	      log("connectionListener: Data received was: " + data);
	   });
	   sock.on("end", function() {
	      log("connectionListener: connection ended!");
	      g_sock = null;
	   });
	}
	var server = net.createServer(connectionListener);
	server.listen(8888);
}


WIFI.disconnect();
WIFI.connect({
	ssid: "sweetie",
	password: "l16wint!",
   network: {
      ip: "192.168.1.99",
      gw: "192.168.1.1",
      netmask: "255.255.255.0"
  }
}, function() {
	log("Connected! - free heap: " + ESP32.getState().heapSize);
	WIFI.setDNS(["8.8.8.8", "8.8.4.4"]);
	createListener();
});