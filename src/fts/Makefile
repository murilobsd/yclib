PROJECT_HOME=../../

include $(PROJECT_HOME)/Make.rules

BIN:= fts

SRC  = $(wildcard *.c)
OBJS = $(patsubst %.c,%.o,$(SRC) )

LIB += -llog -ltransport -lutil -linfo -lexpat -lm -laio -lavformat -lavcodec -lswscale -lswresample -lavutil

all:$(BIN)
$(BIN) : $(OBJS)
	gcc $(CFLAGS) $(INC) $(LIBDIR) $(LIB) $(OBJS) -o $@
	mv $(BIN) $(PROJECT_HOME)/bin/
	rm -rf *.o
%.o:%.c
	gcc $(CFLAGS) $(INC)  -c $< -o $@
                
                
clean:
	rm -rf *.o $(PROJECT_HOME)/bin/$(BIN)
                                              
