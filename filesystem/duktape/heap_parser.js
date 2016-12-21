function processHeapDump(dataString) {
	var TYPE = {
		STRING: 1,
		OBJECT: 2,
		BUFFER: 3
	}
	
	var buffers = [];
	var data = JSON.parse(dataString);
	console.log("We are starting parse");
	console.log("Number of heap items: " + data.heapObjects.length);
	var heapObjs = data.heapObjects;
	var objectCount = heapObjs.length;
	var i;
	var counters = {
		string: 0,
		buffer: 0,
		object: 0,
	}
	var bufferStats = {
		totalBufferLen: 0,
		largestBuffer: 0,
		moreThan256: 0,
		moreThan512: 0,
		moreThan1024: 0
	}
	
	var stringStats = {
		totalStringLen: 0,
		largestString: 0
	}
	for (i=0; i<objectCount; i++) {
		switch(heapObjs[i].type) {
			case TYPE.STRING: {
				counters.string++;
				//console.log(JSON.stringify(heapObjs[i]));
				stringStats.totalStringLen += heapObjs[i].blen;
				if (stringStats.largestString < heapObjs[i].blen) {
					stringStats.largestString = heapObjs[i].blen;
				}
				break;
			}
			case TYPE.BUFFER: {
				counters.buffer++;
				bufferStats.totalBufferLen += heapObjs[i].len;
				buffers.push({len: heapObjs[i].len, data: heapObjs[i].data.data});
				if (heapObjs[i].len > 1024) {
					bufferStats.moreThan1024++;
				} else if (heapObjs[i].len > 512) {
					bufferStats.moreThan512++;
				} else if (heapObjs[i].len > 256) {
					bufferStats.moreThan256++;
				}
				if (bufferStats.largestBuffer < heapObjs[i].len) {
					bufferStats.largestBuffer = heapObjs[i].len;
				}
				break;
			}
			case TYPE.OBJECT: {
				counters.object++;
				break;
			}
		}
	}
	console.log("Counters: " + JSON.stringify(counters));
	console.log("Buffers: " + JSON.stringify(bufferStats));
	console.log("Strings: " + JSON.stringify(stringStats));
	buffers.sort(function(a, b) {
		if (a.len > b.len) {
			return -1;
		}
		if (a.len < b.len) {
			return 1;
		}
		return 0;
	});
	for (i=0; i<25; i++) {
		//console.log(JSON.stringify(buffers[i]));
	}
}
var input = "";
process.stdin.on("data", function(chunk) {
	input += chunk;
});
process.stdin.on("end", function() {
	console.log("Got the whole input!");
	processHeapDump(input);
});