# Compiling JavaScript
When ESP32-Duktape is presented with JavaScript source, it is presented to Duktape for execution.

There are a wide range of APIs for compiling code including:

* `duk_compile` - Compile source that is a string on the value stack.
* `duk_compile_file` - Compile a file read by POSIX file I/O.
* `duk_compile_lstring` - Compile source that is passed as a string pointer with explicit length.
* `duk_compile_lstring_filename` - Compile source that is passed as a string pointers with explicit length.
The "name" of a logical source file is taken from the top of the value stack.
* `duk_compile_string` - Compile source that is passed as a string pointer that is NULL terminated.
* `duk_compile_string_filename` - Compile source that is passed as a string pointer that is NULL terminated.
The "name" of a logical source file is taken from the top of the value stack.

and their pcompile companions:

* duk_pcompile - Compile source that is a string on the value stack.
* duk_pcompile_file - Compile a file read by POSIX file I/O.
* duk_pcompile_lstring - Compile source that is passed as a string pointer with explicit length.
* duk_pcompile_lstring_filename - Compile source that is passed as a string pointers with explicit length.
The "name" of a logical source file is taken from the top of the value stack.
* duk_pcompile_string - Compile source that is passed as a string pointer that is NULL terminated.
* duk_pcompile_string_filename - Compile source that is passed as a string pointer that is NULL terminated.
The "name" of a logical source file is taken from the top of the value stack.


And then we have the evaluation functions which evaluate code directly:

* duk_eval
* duk_eval_file
* duk_eval_file_noresult
* duk_eval_lstring
* duk_eval_lstring_noresult
* duk_eval_noresult
* duk_eval_string
* duk_eval_string_noresult

and their peval companions:

* duk_peval
* duk_peval_file
* duk_peval_file_noresult
* duk_peval_lstring
* duk_peval_lstring_noresult
* duk_peval_noresult
* duk_peval_string
* duk_peval_string_noresult
