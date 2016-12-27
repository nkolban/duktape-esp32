/*
 * Partitions module.
 */
var modulePartitions = ESP32.getNativeFunction("ModulePartitions");
if (modulePartitions === null) {
	log("Unable to find ModulePartitions");
	module.exports = null;
	return;
}

var partitions = {	
};

modulePartitions(partitions);

module.exports = partitions;