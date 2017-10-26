PROJECT_HOME=../../../

include $(PROJECT_HOME)/Make.rules


BIN:= x2h

SRC  = $(wildcard *.c)
OBJS = $(patsubst %.c,%.o,$(SRC) )

LIB += 

all:$(BIN)
$(BIN) : $(OBJS)
	gcc $(CFLAGS) $(INC) $(LIBDIR) $(COMMLIB) $(OBJS) -o $@
	mv $(BIN) $(PROJECT_HOME)/bin/
	rm -rf *.o
%.o:%.c
	gcc $(CFLAGS) $(INC) -c $< -o $@
                
                
clean:
	rm -rf *.o $(PROJECT_HOME)/bin/$(BIN)
                                              
