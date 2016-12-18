## _readline_
> Read a file line by line.

## Install

```sh
npm install readline
```

## What's this?

Simple streaming readline module for NodeJS. Reads a file and buffer new lines emitting a _line_ event for each line.

## Usage
```js
  var readline = require('readline'),
      rl = readline('./somefile.txt');
  rl.on('line', function(line) {
    // do something with the line of text
  })
  .on('error', function(e) {
    // something went wrong
  });
```

## License

BSD © [Craig Brookes](http://craigbrookes.com/)
