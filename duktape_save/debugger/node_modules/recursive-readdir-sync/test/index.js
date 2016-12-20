var chai = require('chai')
  , expect = chai.expect
  , _ = require('lodash')
  , path = require('path')
  , recursiveReaddirSync = require('../index')
  ;

describe('Functionality testing.', function(){
  //Files we should find in or path
  var expectedFiles = [
    './nested/a/file1.txt',
    './nested/a/b/file1.txt',
    './nested/a/b/file2.js',
    './nested/a/b/.hidden1',
    './nested/x/.hidden2',
    './nested/y/z/file.conf'
  ];

  expectedFiles = _.map(expectedFiles, function(f){
    return path.resolve(__dirname, f);
  });

  //Directories should not be listed in the results only files.
  var unexpectedFiles = [
    './nested/empty'
  ];

  unexpectedFiles = _.map(unexpectedFiles, function(f){
    return path.resolve(__dirname, f);
  });

  var results = recursiveReaddirSync(__dirname + '/nested');

  it('should return an array with length equal to that of expectedFiles', function(){
      expect(results).to.have.length(expectedFiles.length);
  });

  it('should find all nested files in the folder structure', function(){
     expect(_.xor(results, expectedFiles)).to.have.length(0);
  });

  it('should not contain empty folders', function(){
    expect(results).to.not.include.members(unexpectedFiles);
  });

});





