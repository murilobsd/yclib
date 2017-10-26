PROJECT_HOME=../../

include $(PROJECT_HOME)/Make.rules

BIN:= yms_daemon

SRC  = $(wildcard *.c)
OBJS = $(patsubst %.c,%.o,$(SRC) )

LIB += -llog -lmedia -ltransport -lutil

all:$(BIN)
$(BIN) : $(OBJS)
	gcc $(CFLAGS) $(INC) $(LIBDIR) $(OBJS) -o $@
	mv $(BIN) $(PROJECT_HOME)/lib/
	rm -rf *.o
%.o:%.c
	gcc $(CFLAGS) $(INC)  -c $< -o $@
                
                
clean:
	rm -rf *.o $(PROJECT_HOME)/lib/$(BIN)
                                              
