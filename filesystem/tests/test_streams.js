/**
 * Test the streams module.
 */

var stream = require("stream");


function stream_test1() {
	var pass = 0;
	var fail = 0;
	var testsRun = 0;
	console.log("test - streams");
	var dataItem1 = "1234567890";
	var dataItem2 = "ABCDEFGHIJ";
	
	
	var s1 = new stream();
	
	// #1
	// Let us test a simple read/write with a write before a read.
	s1.writer.write(dataItem1);
	var read = s1.reader.read();
	if (read.toString() !== dataItem1) {
		console.log("Error: test1 failed");
		fail++;
	} else {
		pass++;
	}
	testsRun++;
	
	// #2 - test a continued write/read.
	s1.writer.write(dataItem2);
	var read = s1.reader.read();
	if (read.toString() !== dataItem2) {
		console.log("Error: test2 failed");
		fail++;
	} else {
		pass++;
	}
	testsRun++;
	
	// #3 - test a callback for a read.
	s1.reader.on("data", function(data) {
		if (data.toString() === dataItem1) {
			pass++;
		} else {
			console.log("Error: test3 failed");
			fail++;
		}
	});
	s1.writer.write(dataItem1);
	testsRun++;
	
	// #4 Test that a writer.end() is caught.
	s1.reader.on("end", function() {
		pass++;
	});
	s1.writer.end();
	testsRun++;
	
	// #5 Test that a write.end(data) sends the data.
	s2 = new stream();
	s2.reader.on("data", function(data) {
		if (data.toString() === dataItem1) {
			pass++;
		} else {
			console.log("Error: test5 failed");
			fail++;
		}
	});
	s2.writer.end(dataItem1);
	testsRun++;
	
	setTimeout(function() {
		console.log("Tests complete: Pass: " + pass + ", Fail: " + fail + " off " + testsRun);
	}, 500);
}

stream_test1();