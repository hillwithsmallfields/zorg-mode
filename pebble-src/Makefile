CFLAGS=-g -c -Wall
LDFLAGS=
UTILSOURCES=zorg-data.c zorg-actions.c zorg-files.c zorg-stream.c
TOPLEVELS=zorg-pebble.c zorg-cmd.c
UTILOBJECTS=$(UTILSOURCES:.c=.o)
COMMAND=zorg

all: $(COMMAND) zorg-unit-test

$(COMMAND): zorg-cmd.o $(UTILOBJECTS)
	$(CC) $(LDFLAGS) zorg-cmd.o $(UTILOBJECTS) -o $@

zorg-unit-test: zorg-unit-test.o $(UTILOBJECTS)
	$(CC) $(LDFLAGS) zorg-unit-test.o $(UTILOBJECTS) -o $@

.c.o: $< zorg.h
	$(CC) $(CFLAGS) $< -o $@
