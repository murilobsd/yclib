PROJECT_HOME=../../

include $(PROJECT_HOME)/Make.rules

BIN:=liblog.so

SRC  = $(wildcard *.c)
OBJS = $(patsubst %.c,%.o,$(SRC) )

all:$(BIN)
$(BIN) : $(OBJS)
	gcc $(CFLAGS) $(INC) $(LIBDIR) $(OBJS) -shared -o $@
	mv $(BIN) $(PROJECT_HOME)/lib/
	rm -rf *.o
%.o:%.c
	gcc $(CFLAGS) $(INC) -fPIC -c $< -o $@
                
                
clean:
	rm -rf *.o $(PROJECT_HOME)/lib/$(BIN)
                                              
