CFLAGS=-c -Wall
LDFLAGS=
SOURCES=zorg-pebble.c zorg-data.c zorg-actions.c zorg-files.c zorg-stream.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=zorg

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.cpp.o: zorg.h
	$(CC) $(CFLAGS) $< -o $@
