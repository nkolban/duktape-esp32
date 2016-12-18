var test = require("tap").test;
var readLine = require('../readline.js');
test("test reading lines",function(t){
   console.error("reading large file line by line asserts may take a while");
   var rl = readLine('./fixtures/afile.txt');
   rl.on("line", function (line){
     t.ok(null !== line && undefined !== line);
   });
   rl.on("end",function (){
   	t.end();
   });
   
});

test("numbers", function (t){
   var rl = readLine('./fixtures/nmbr.txt');
   var answer = 28;
   var i=0;
   rl.on("line", function (line){
   	 	var num = Number(line);
   	 	console.error(num);
        i+=num;

   });
   rl.on("end", function (){
   	console.error(i,answer);
   t.ok(answer === i, "I is wrong " + i);
   t.end();
   });
});


test("errors", function (t){
	var rl = readLine("./Idontexist");
    rl.on('error', function (e){
      t.ok(e);
      t.end();
    });
    rl.on('end', function (){
    	t.end();
    });
    rl.on('close', function(){
     t.end();
    });
});


