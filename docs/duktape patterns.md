## Creating an array
To create an empty array, we call:

```
duk_idx_t idx = duk_push_array(ctx);
```

To insert an item, we push the item onto the stack and then call

```
duk_put_prop_index(ctx, arrayObjIndex, arrayIndex);
```

After adding the item into the array, it is removed from the top of the stack.

##Creating a new object
To create a new empty object w call:

```
duk_idx_t duk_push_object(ctx)
```

## Setting a property on an object
To set a property on an object we assume that the object is on the value stack at 
stack index `objIdx`. Next we push the value we wish to set onto the end of the stack.
Finally we call:

```
duk_put_prop_string(ctx, objIdx, keyString);
```

The value is removed from the stack and is now part of the object keyed by `keyString`.

## Getting a property off an object
To get a property of an object we assume that the object is on the value stack at
stack index `objIdx`.  Now we call:

```
duk_get_prop_string(ctx, objIdx, keyString);
```

The property with the name `keyString` is now at the top of the stack.


## Removing a property off an object
To remove a property from an object, we assumed that the object is on the value stack
at stack index `objIdx`.  Now we call:

```
duk_del_prop_string(ctx, objIfx, keyString);
```

## Stack manipulation
To get the number of items on the stack, call:
```
duk_idx_t count = duk_get_top(ctx);
```

To get the index of the top item on the stack call:
```
duk_idx_t topIdx = duk_get_top_index(ctx);
```
The value will be `DUK_INVALID_INDEX` if the stack is empty and there is no top item.

To duplicate a stack item, call:
```
duk_dup(ctx, idx);
```
The stack item found at `idx` will be duplicated to the top of the stack.

To duplicate the top stack item, call:
```
duk_dup_top(ctx);
```

To move the value at the top of the stack to a new location in the stack we call:
```
duk_insert(ctx, idx);
```
What happens here is that the top item is removed from the stack and placed into the stack such that it becomes the `idx`
entry.  All stack items that were previously at `idx` or above are moved up by 1.

To swap two items in the stack, call
```
duk_swap(ctx, index1, index2);
```

To remove an element from the stack we can call:
```
duk_remove(ctx, idx);
```

## Getting the data and size of a Buffer
We can use `duk_get_buffer()` to retrieve the data and size of the buffer.
```
void *duk_get_buffer_data(ctx, index, &size)
```

Returns NULL on a problem.


## Creating and populating a buffer
Imagine we have some data where we know its size and want to return this from C to JS
as a Buffer.

```
void *data = duk_push_fixed_buffer(ctx, size);
memcpy(data, original, size);
duk_push_buffer_object(ctx, -1, 0, size, DUK_BUFOBJ_NODEJS_BUFFER);
```

## Throwing an error from C
```
duk_error(ctx, code, fmtString, ...);
```

The code should be > 0.