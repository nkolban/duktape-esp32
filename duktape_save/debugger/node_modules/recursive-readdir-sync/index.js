var fs = require('fs')
  , p = require('path')
  ;

function recursiveReaddirSync(path) {
  var list = []
    , files = fs.readdirSync(path)
    , stats
    ;

  files.forEach(function (file) {
    stats = fs.lstatSync(p.join(path, file));
    if(stats.isDirectory()) {
      list = list.concat(recursiveReaddirSync(p.join(path, file)));
    } else {
      list.push(p.join(path, file));
    }
  });

  return list;
}

module.exports = recursiveReaddirSync;
