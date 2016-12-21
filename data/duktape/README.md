# ESP32-Duktape ./data/duktape directory
This is a data directory that contains files necessary for building ESP32-Duktape.
These include:

* `component.mk` - ESP-IDF Makefile for building the `./components/duktape` component.  Should be
copied into the `./components/duktape` directory.
* `ESP32-Duktape.yaml` - A configuration file in YAML format that is used (in part) to drive the
configuration of the profile for the version of Duktape we will be using.