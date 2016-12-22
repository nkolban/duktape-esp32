/* globals require, log, WIFI, DUKF, ESP32, module */
var NVS  = require("nvs.js");
var HTTP = require("http.js");
var URL  = require("url.js");

/*
 *  Start a WebServer on port 80.  The server will serve up the access point
 *  settings page that allows the user to supply the details of the access
 *  point to which we will connect on subsequent boots.  These details are
 *  saved in NVS.
 */
function startWebServer() {
	log("Starting the bootwifi Web Server");
	function requestHandler(request, response) {
		var postData = "";
		
	   log("We have received a new HTTP client request!");
	   // Accumulate the posted data that will be the form.
	   request.on("data", function(data) {
	      log("HTTP Request handler: " + data);
	      postData += data;
	   });
	   
	   // Handle the end of the receipt of the HTTP request.
	   request.on("end", function() {
	      log("HTTP request received:");
	      log("- path: " + request.path);
	      log("- method: " + request.method);
	      log("- headers: " + JSON.stringify(request.headers));
	      // Return the web page for data entry.
	      if (request.path == "/") {
	      	// We have received a request to load the main page.
	      	var htmlText = DUKF.loadFile("/web/selectAP.html");
	      	response.writeHead(200);
	      	response.write(htmlText);
	      	response.end();
	      }
	      // POST request received for form handling.
	      else if (request.path == "/ssidSelected" && request.method == "POST") {
	      	// We have received a response form page ... now we parse it.
	      	log("Parsing response form ...");
	      	var formObj = URL.queryParse(postData);
	      	log("Form parts: " + JSON.stringify(formObj));
	      	var bootWiFi_ns = NVS.open("bootwifi", "readwrite");
	      	bootWiFi_ns.set("ssid",     formObj.ssid,     "string");
	      	bootWiFi_ns.set("password", formObj.password, "string");
	      	bootWiFi_ns.set("ip",       formObj.ip,       "string");
	      	bootWiFi_ns.set("gw",       formObj.gw,       "string");
	      	bootWiFi_ns.set("netmask",  formObj.netmask,  "string");
	      	bootWiFi_ns.close();
	      	
	      	response.writeHead(200);
	      	response.write("Got data - rebooting in 5");
	      	response.end();
	      }
	      // Not a path that we handle.
	      else {
	      	response.writeHead(404);
	      	response.end();
	      }
	   });
	}
	
	// Create the HTTP server with the given request handler and start the server
	// listening on the default HTTP port.
	var server = HTTP.createServer(requestHandler);
	server.listen(80);
} // startWebServer 

function becomeAccessPoint() {
	log("Becoming an access point");
	WIFI.start();
	WIFI.listen({
		ssid: "esp32-duktape",
		auth: "open"
	}, function() {
		log("Wifi Callback!!!");
		// We are now a WiFi access point ... so let us log our IP address
		log("***********************");
		log("* Now an Access Point *");
		log("***********************");
		log("IP Address: " + WIFI.getState().apIp);
		
		// and start a WebServer
		startWebServer();
	});
} // becomeAccessPoint()


function boot(bootedCallback) {
	var bootWiFi_ns = NVS.open("bootwifi", "readwrite");
	
	try {
		// Retrieve any saved NVS values.
		var ssid     = bootWiFi_ns.get("ssid", "string");
		var password = bootWiFi_ns.get("password", "string");
		var ip       = bootWiFi_ns.get("ip", "string");
		var gw       = bootWiFi_ns.get("gw", "string");
		var netmask  = bootWiFi_ns.get("netmask", "string");
		log("Going to try and connect to AP named \"" +  ssid + "\"");
		log("Network info is: ip=%s, gw=%s, netmask=%s", ip, gw, netmask);
		log("ESP32 Heap: " + ESP32.getState().heapSize);
		
		WIFI.connect({
			ssid: ssid,
			password: password,
			network: {
				ip: ip,
				gw: gw,
				netmask: netmask
			}
		}, function(err) {
				log("Now connected as a station! - err: " + err);
				if (err !== null) {
					log("Performing a stop of WiFi");
					WIFI.stop();
					becomeAccessPoint();
				} else {
					log("Onwards ... we are now network connected!");
					bootedCallback();
				}
				log("ESP32 Heap: " + ESP32.getState().heapSize);
			}
		);
	}
	catch(e) {
		log("bootwifi: caught an exception: " + e.stack);
		becomeAccessPoint();
	}

	bootWiFi_ns.close();
	log("ESP32 Heap: " + ESP32.getState().heapSize);
} // boot()

module.exports = boot;