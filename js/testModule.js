var name = "myModule";
module.exports.f1 = function() {
	log("Hello from f1");
}
function f2() {
	log("Hello from local variable which contains: " + name);
}
module.exports.f2 = f2;
log("Module loaded and functions f1() and f2() exposed");