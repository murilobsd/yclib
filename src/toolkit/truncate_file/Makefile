PROJECT_HOME=../../../

include $(PROJECT_HOME)/Make.rules


BIN:= truncate_file

SRC  = $(wildcard *.c)
OBJS = $(patsubst %.c,%.o,$(SRC) )

LIB += -lutil -lpthread

all:$(BIN)
$(BIN) : $(OBJS)
	gcc $(CFLAGS) $(INC) $(LIBDIR) $(COMMLIB) $(OBJS) ${LIB} -o $@
	mv $(BIN) $(PROJECT_HOME)/bin/
	rm -rf *.o
%.o:%.c
	gcc $(CFLAGS) $(INC) -c $< -o $@
                
                
clean:
	rm -rf *.o $(PROJECT_HOME)/bin/$(BIN)
                                              
