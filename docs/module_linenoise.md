# linenoise module
User input from console (stdin/stdout). Useful for interactive commands with
editing and line history. It's a modified version of the esp-idf copy of
linenoise written by someone else (see the attribution in module_linenoise.c).

We need to poll to give it a chance to process user input. The poll function
will return a complete string once one has been typed.

## Example:

if(true) {
	var linenoise = require('linenoise');
	linenoise.setPrompt('test> ');
	setInterval(function() {
		var line = linenoise.poll();
		if(line!==undefined) linenoise.print(line);
	}, 20);
}
