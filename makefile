CC = cc

CFLAGS  = -std=c99
CFLAGS += -pedantic
CFLAGS += -Wall
CFLAGS += -Wextra
CFLAGS += -Wcast-align
CFLAGS += -Wswitch-default
CFLAGS += -Wstrict-prototypes
CFLAGS += -Wundef

CFLAGS += -D_DEFAULT_SOURCE

PREFIX = /usr/local
INCLUDES = -I/usr/local/include
LIBS = -L/usr/local/lib -lmarkdown

PROJECT = swege

all:
	$(CC) $(CFLAGS) $(INCLUDES) $(LIBS) *.c -o $(PROJECT)

debug: 
	$(CC) $(CFLAGS) -g $(INCLUDES) $(LIBS) *.c -o $(PROJECT)

gdb: debug
	gdb $(PROJECT)

memcheck: all
	valgrind --leak-check=yes ./$(PROJECT)

memcheck_v: all
	valgrind --leak-check=yes -v ./$(PROJECT)

clean:
	rm $(PROJECT)

install:
	cp $(PROJECT) $(PREFIX)/bin/$(PROJECT)

site:
	./$(PROJECT)
