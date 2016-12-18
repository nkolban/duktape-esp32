# Makefile for DUKF Linux build
# We want the ability to build the DUKF project on a Linux environment and this
# Makefile does just that.
#

OBJS=../main/duktape_task.o \
../main/modules.o \
../main/duktape_event.o \
../main/duktape_utils.o \
../main/logging.o \
../main/module_fs.o \
../main/module_os.o \
../main/module_dukf.o \
../main/dukf_utils.o \
../main/c_timeutils.o \
main.o \
../components/duktape/examples/debug-trans-socket/duk_trans_socket_unix.o \
../components/duktape/src/duktape.o \
../components/duktape/extras/module-duktape/duk_module_duktape.o
LOBJS=duktape_task.o main.o duktape.o modules.o duktape_utils.o duktape_event.o module_fs.o \
dukf_utils.o \
logging.o \
c_timeutils.o \
module_os.o \
module_dukf.o \
duk_trans_socket_unix.o \
duk_module_duktape.o
CFLAGS=-g
INCLUDES=-I../components/duktape/src -I../components/duktape/extras/module-duktape \
-I../components/duktape/examples/debug-trans-socket \
-I../main
LIBS=-lm -lcrypto

all: $(OBJS)
	@echo "CC -o dtest"
	@$(CC) -o dtest $(LOBJS) $(LIBS)


.c.o:
	@echo "CC $<"
	@$(CC) -c $(CFLAGS) $< $(INCLUDES)
	
clean:
	rm -f $(LOBJS)