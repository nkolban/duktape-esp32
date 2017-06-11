# Modules
At the highest level, we load a module using:
`var mod = require("<mod name>");`

A module is a JavaScript program that will be loaded from a file.  The context of this program is rather special.

* Variables defined in the context will be scoped to just the module.
* A variable called "exports" is predefined which is the the "value" of the module.  In the later releases of Duktape,
the "exports" variable is now part of "module" as in "module.exports".
* A variable called "module" provides metadata.

Using the require() function also appears to cache data.  So if we perform a `require("x")` and a subsequent
`require("x")` we don't load it twice but rather the previous load is re-used.

## Loading modules
To allow Duktape to find modules, we must provide a function called `Duktape.modSearch`.  This function takes as 
a parameter the name of the module to locate and returns the source code of the module.  The full signature for
Duktape.modSearch is

`function(id, require, exports, module)`

Typically what we want to do is use the `id` value as the file name and perform a file read.  If we have the `FS` module 
pre-loaded, this might take the shape of:

```
Duktape.modSearch = function(id, require, exports, module) {
   var fd = FS.openSync("/spiffs/" + id);
   var stat = FS.fstatSync(fd);
   var data = new Buffer(stat.size);
   FS.readSync(fd, data, 0, data.length, null);
   FS.closeSync(fd);
   return data.toString();
}
```

To make this easier, we created a native function called `ESP32.loadFile(path)` that loads the named file
from the local POSIX file system.  So our logic becomes:

```
Duktape.modSearch = function(id, require, exports, module) {
   return ESP32.loadFile("/spiffs/" + id");
}
```


## References
* Duktape guide - [Modules](http://duktape.org/guide.html#modules)
* Detauils on modules - [modules.rst](https://github.com/svaarala/duktape/blob/master/doc/modules.rst)
* [CommonJS modules](http://wiki.commonjs.org/wiki/Modules/1.1.1)
* Duktape C module convention - [c-module-convention.rst](https://github.com/svaarala/duktape/blob/master/doc/c-module-convention.rst)
* Duktape Wiki - [How to use modules](http://wiki.duktape.org/HowtoModules.html)