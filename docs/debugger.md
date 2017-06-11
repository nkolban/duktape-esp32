# Debugger
Duktape has built in debugging technology.

To enable we need to switch on:

* DUK_USE_DEBUGGER_SUPPORT
* DUK_USE_INTERRUPT_COUNTER

See also:
 * [Duktape debugger](https://github.com/svaarala/duktape/blob/master/doc/debugger.rst)
 * https://github.com/svaarala/duktape/tree/master/debugger
 
 To enable heap debugging, we need to compile with DUK_OPT_DEBUGGER_DUMPHEAP
 
 To attach a brower we run:
 
 http://localhost:9092
 
 Make sure you use the same version of the debugger as the Duktape library.