# Makefile for DUKF Linux build
# We want the ability to build the DUKF project on a Linux environment and this
# Makefile does just that.
#

TARGET:=esp32-duktape-linux

OBJS:=\
c_timeutils.o \
duk_trans_socket_unix.o \
duk_module_duktape.o \
dukf_utils.o \
duktape.o \
duktape_event.o \
duktape_task.o \
duktape_utils.o \
logging.o \
main.o \
modules.o \
module_dukf.o \
module_fs.o \
module_os.o


CFLAGS:=-g
INCLUDES:=\
-I../components/duktape/src \
-I../components/duktape/extras/module-duktape \
-I../components/duktape/examples/debug-trans-socket \
-I../main/include
LIBS:=-lm -lcrypto

define cc-command
@echo "CC $<"
@$(CC) -c $(CFLAGS) $< $(INCLUDES)
endef


all: $(TARGET)

$(TARGET): $(OBJS)
	@echo "CC -o $(TARGET)"
	@$(CC) -o $(TARGET) $(OBJS) $(LIBS)

c_timeutils.o: ../main/c_timeutils.c
	$(cc-command)	

duk_trans_socket_unix.o: ../components/duktape/examples/debug-trans-socket/duk_trans_socket_unix.c
	$(cc-command)
		
duk_module_duktape.o: ../components/duktape/extras/module-duktape/duk_module_duktape.c
	$(cc-command)
	
dukf_utils.o: ../main/dukf_utils.c
	$(cc-command)

duktape.o: ../components/duktape/src/duktape.c
	$(cc-command)	

duktape_event.o: ../main/duktape_event.c
	$(cc-command)
	
duktape_task.o: ../main/duktape_task.c
	$(cc-command)

duktape_utils.o: ../main/duktape_utils.c
	$(cc-command)

logging.o: ../main/logging.c
	$(cc-command)
		
main.o: main.c
	$(cc-command)	
	
modules.o: ../main/modules.c
	$(cc-command)

module_dukf.o: ../main/module_dukf.c
	$(cc-command)

module_fs.o: ../main/module_fs.c
	$(cc-command)

module_os.o: ../main/module_os.c
	$(cc-command)	
	
.c.o:
	@echo "CC $<"
	@$(CC) -c $(CFLAGS) $< $(INCLUDES)
	
clean:
	rm -f $(OBJS)
	rm -f $(TARGET)
