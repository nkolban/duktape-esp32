# Design for modules
Duktape has built in support for modules via the `requires()` function.  The idea here is
that a JS programmer can code:

```
var someModule = require("SOMEMODULE");
```

and the module will be dynamically loaded and returned via the object `someModule`.

Since the loading of modules is environment specific, Duktape provides an extensible architecture.  When a
request to load a module is performed, a call is made to the JavaScript function found at `Duktape.modSearch`.
To implement a custom loader, we thus provide an implementation for this function.  The signature of the
function is:

```
Duktape.modSearch = function(id, require, exports, module);
```

Where

* `id` - The identity (string) of the module to be loaded.
* `require` - ??
* `exports` - ??
* `module` - ??

The return from the function should be a string that contains the JavaScript that implements the module.



## See also

* [Duktape Modules](https://github.com/svaarala/duktape/blob/master/doc/modules.rst)