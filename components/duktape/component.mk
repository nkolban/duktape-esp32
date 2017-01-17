# ESP-IDF component.mk file for component "duktape"
CFLAGS+=-Wno-unused-value
COMPONENT_ADD_INCLUDEDIRS:=src extras/module-duktape examples/debug-trans-socket
COMPONENT_SRCDIRS:=src extras/module-duktape examples/debug-trans-socket