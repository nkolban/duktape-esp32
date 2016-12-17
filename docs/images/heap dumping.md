#Heap Dumping with Duktape
Using duktape, we can request a heap dump that is returned as a JSON object.  The object is big and
we need to do something useful with it.

The object returned has the format:

```
{
   type: "heapdump",
   heapObjects: [
      <a heap object>
   ]
}
```

A heap object is a record who's structure varies with the type of object.  It will always have:

```
{
   type: <heaphdr_type - a DUK_HTYPE_xxx>
   flags: <heaphdr_flags>
   refc: <refcount>
}
```

The types are:
* 0/1 - String
* 1/2 - Object
* 2/3 - Buffer

For an string we have:
```
{
   type: 1
   flags: <heaphdr_flags>
   refc: <refcount>
   blen: <bytelen - Size of string>
   data: <string data>
}
```

For a object we have:
```
{
   type: 2
   flags: <heaphdr_flags>
   refc: <refcount>
   esize: <e_size> - The entry part size.
}

For a buffer we have:
```
{
   type: 3
   flags: <heaphdr_flags>
   refc: <refcount>
   len: <length of buffer>
}
```

See also:
* duktape/src-input/duk_heaphdr.h