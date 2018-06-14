var linenoise = require('linenoise');
linenoise.setPrompt('test> ');
setInterval(function() {
	var line = linenoise.poll();
	if(line!==undefined) {
		if(line=='reboot' || line=='reset') ESP32.reboot();
		linenoise.print(line);
		try {log(eval(line));}
		catch(e) {log(e);}
	}
}, 20);
