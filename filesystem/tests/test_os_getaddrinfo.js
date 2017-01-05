var hostname = "www.neilkolban.com";

var address = OS.getaddrinfo(hostname);
log("Result of looking up " + hostname + " is " + address);
log("Result of looking up " + address + " is " + OS.getaddrinfo(address));