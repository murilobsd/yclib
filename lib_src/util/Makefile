PROJECT_HOME=../../

include $(PROJECT_HOME)/Make.rules

BIN:=libutil.so

SRC  = $(wildcard *.c)
OBJS = $(patsubst %.c,%.o,$(SRC) )

LIB += -laio -lanl

all:$(BIN)
$(BIN) : $(OBJS)
	gcc $(CFLAGS) $(INC) $(LIBDIR) $(OBJS) $(LIB) -shared -o $@
	mv $(BIN) $(PROJECT_HOME)/lib/
	rm -rf *.o
%.o:%.c
	gcc $(CFLAGS) -D_GNU_SOURCE $(INC) -fPIC -c $< -o $@
                
                
clean:
	rm -rf *.o $(PROJECT_HOME)/lib/$(BIN)
                                              
