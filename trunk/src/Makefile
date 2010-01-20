
EXE = rpl-simulator
OBJS = main.o node.o system.o gui/mainwin.o gui/simfield.o proto/phy.o proto/mac.o proto/ip.o proto/rpl.o
CFLAGS = -Wall -g3 -pthread -std=gnu99 `pkg-config --cflags gtk+-2.0 gthread-2.0`
LDFLAGS = -Wall -rdynamic -pthread -lm `pkg-config --libs gtk+-2.0 gthread-2.0 gmodule-export-2.0`

CC = gcc
RM = rm -f

all: $(EXE)

clean:
	$(RM) $(OBJS) $(EXE)

$(EXE) : $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $@

.o:
	$(CC) -c $< $(CFLAGS) -o $@

main.o: main.c main.h base.h node.h system.h proto/phy.h proto/mac.h proto/ip.h proto/rpl.h gui/mainwin.h

node.o: node.c node.h base.h system.h proto/phy.h proto/mac.h proto/ip.h proto/rpl.h

system.o: system.c system.h base.h node.h proto/phy.h proto/mac.h proto/ip.h proto/rpl.h

gui/mainwin.o: gui/mainwin.c gui/mainwin.h base.h node.h main.h system.h gui/simfield.h proto/phy.h proto/mac.h proto/ip.h proto/rpl.h

gui/simfield.o: gui/simfield.c gui/simfield.h base.h node.h main.h system.h proto/phy.h proto/mac.h proto/ip.h proto/rpl.h 

proto/phy.o: proto/phy.c proto/phy.h base.h node.h system.h proto/mac.h proto/ip.h proto/rpl.h

proto/mac.o: proto/mac.c proto/mac.h base.h node.h system.h proto/phy.h proto/ip.h proto/rpl.h

proto/ip.o: proto/ip.c proto/ip.h base.h node.h proto/phy.h proto/mac.h proto/rpl.h

proto/rpl.o: proto/rpl.c proto/rpl.h base.h node.h system.h proto/phy.h proto/mac.h proto/ip.h 
