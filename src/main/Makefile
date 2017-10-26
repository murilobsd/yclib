PROJECT_HOME=../../

include $(PROJECT_HOME)/Make.rules

BIN:= yms

SRC  = $(wildcard *.c)
OBJS = $(patsubst %.c,%.o,$(SRC) )

LIB += -llog -lmedia -ltransport -lutil -linfo -lpthread -lexpat -lm -laio

all:$(BIN)
$(BIN) : $(OBJS)
	gcc $(CFLAGS) $(INC) $(LIBDIR) $(LIB) $(OBJS) -o $@
	mv $(BIN) $(PROJECT_HOME)/bin/
	rm -rf *.o
%.o:%.c
	gcc $(CFLAGS) $(INC)  -c $< -o $@
                
                
clean:
	rm -rf *.o $(PROJECT_HOME)/bin/$(BIN)
                                              
