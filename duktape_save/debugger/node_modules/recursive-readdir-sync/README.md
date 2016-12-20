recursive-readdir-sync
======================
NodeJS library to recursively read a directory path's contents synchronously

A simple Node module for synchronously listing all files in a directory, or in any subdirectories.

It does not list directories themselves.

This library uses synchronous filesystem calls. That means this library uses **BLOCKING** calls. Keep that in mind
when using it.

Install
-------

```
npm install recursive-readdir-sync
```

Example
-------
```javascript
var recursiveReadSync = require('recursive-readdir-sync')
  , files
  ;

try {
  files = recursiveReadSync('/your/path/here');
} catch(err){
  if(err.errno === 34){
    console.log('Path does not exist');
  } else {
    //something unrelated went wrong, rethrow
    throw err;
  }
}

console.log('Files array:', files);

//loop over resulting files
for(var i = 0, len = files.length; i < len; i++){
  console.log('Found: %s', files[i]);
}
```
