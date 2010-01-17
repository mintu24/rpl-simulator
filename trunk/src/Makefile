
EXE = rpl-simulator
OBJS = main.o node.o system.o gui/mainwin.o gui/simfield.o proto/phy.o proto/mac.o proto/ip.o proto/rpl.o
CFLAGS = -Wall -g3 -pthread `pkg-config --cflags gtk+-2.0 gthread-2.0`
LDFLAGS = -Wall -rdynamic -pthread `pkg-config --libs gtk+-2.0 gthread-2.0 gmodule-export-2.0`

CC = gcc
RM = rm -f

all: $(EXE)

clean:
	$(RM) $(OBJS) $(EXE)

$(EXE) : $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $@

.o:
	$(CC) -c $< $(CFLAGS) -o $@

main.o: main.c base.h gui/mainwin.h

node.o: node.c node.h base.h

system.o: system.c system.h base.h node.h

gui/mainwin.o: gui/mainwin.c gui/mainwin.h base.h gui/simfield.h

gui/simfield.o: gui/simfield.c gui/simfield.h base.h

proto/phy.o: proto/phy.c proto/phy.h base.h node.h proto/mac.h

proto/mac.o: proto/mac.c proto/mac.h base.h node.h proto/ip.h

proto/ip.o: proto/ip.c proto/ip.h base.h node.h proto/rpl.h

proto/rpl.o: proto/rpl.c proto/rpl.h base.h node.h
