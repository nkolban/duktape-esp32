log('This is ' + module.filename);
var treeify=require('treeify.js').asTree;
function show(x) {log(treeify(x, true));}
//show(this);
DUKF.gc();
log('Heapsize: ' + ESP32.getState().heapSize);
