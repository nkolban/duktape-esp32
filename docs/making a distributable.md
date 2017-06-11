# Making a distributable
After you have built ESP32-Duktape from source, you may want to create a distributable
which is a packaged version of the resulting binaries.  The Makefile contained in the
root of the project has a target called `ship`.  We run that using:

```
$ make ship
```

A compilation of the source and creation of the filesystem images is performed and the
resulting binaries are then packaged together.   The result is a file with the name:
`esp32-duktape-YYYY-MM-DD.tar.gz`.  This is the file that can be distributed to others.

It can be extracted by the recipients using the command:

```
$ tar -xvzf esp32-duktape-YYYY-MM-DD.tar.gz
```

which will extract the content into the current directory.  The resulting files will be:

* `bootloader.bin`
* `esp32-duktape.bin`
* `espfs.img`
* `partitions_singleapp.bin`
* `spiffs.img`

From there we can follow the installation instructions that are described in [installation](installation.md).