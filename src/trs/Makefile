PROJECT_HOME=../../

include $(PROJECT_HOME)/Make.rules

BIN:= trs

SRC  = $(wildcard *.c)
OBJS = $(patsubst %.c,%.o,$(SRC) )

LIB += -ldb  -ltransport -lutil -lpthread -lexpat -lm -laio -linfo -llog -lscreen

all:$(BIN)
$(BIN) : $(OBJS)
	gcc $(CFLAGS) $(INC) $(LIBDIR) $(LIB) $(OBJS) -Wl,-rpath $(PROJECT_HOME)/lib/ -o $@
	mv $(BIN) $(PROJECT_HOME)/bin/
	rm -rf *.o
%.o:%.c
	gcc $(CFLAGS) $(INC)  -c $< -o $@
                
                
clean:
	rm -rf *.o $(PROJECT_HOME)/bin/$(BIN)
                                              
