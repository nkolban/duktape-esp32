#Modules
At the highest level, we load a module using:
`var mod = require("<mod name>");`

A module is a JavaScript program that will be loaded from a file.  The context of this program is rather special.

* Variables defined in the context will be scoped to just the module.
* A variable called "exports" is predefined which is the the "value" of the module.  In the later releases of Duktape,
the "exports" variable is now part of "module" as in "module.exports".
* A variable called "module" provides metadata.

##Loading modules
To allow Duktape to find modules, we must provide a function called "Duktape.modSearch".  This function takes as 
a parameter the name of the module to locate and returns the source code of the module.  The full signature for
Duktape.modSearch is

`function(id, require, exports, module)`


##References
* Duktape guide - [Modules](http://duktape.org/guide.html#modules)
* Detauils on modules - [modules.rst](https://github.com/svaarala/duktape/blob/master/doc/modules.rst)
* [CommonJS modules](http://wiki.commonjs.org/wiki/Modules/1.1.1)
* Duktape C module convention - [c-module-convention.rst](https://github.com/svaarala/duktape/blob/master/doc/c-module-convention.rst)
* Duktape Wiki - [How to use modules](http://wiki.duktape.org/HowtoModules.html)