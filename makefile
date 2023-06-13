CC ?= cc

CFLAGS  = -std=c99
CFLAGS += -pedantic
CFLAGS += -Wall
CFLAGS += -Wextra
CFLAGS += -Wcast-align
CFLAGS += -Wswitch-default
CFLAGS += -Wstrict-prototypes
CFLAGS += -Wundef

CFLAGS += -D_DEFAULT_SOURCE

PREFIX ?= /usr/local
INCLUDES = -I/usr/local/include
LIB_PATH = -L/usr/local/lib
LIBS = -lmarkdown

PROJECT = swege

all:
	$(CC) $(CFLAGS) -O3 $(INCLUDES) $(LIB_PATH) *.c -o $(PROJECT) $(LIBS)

debug: 
	$(CC) $(CFLAGS) -g $(INCLUDES) $(LIB_PATH) *.c -o $(PROJECT) $(LIBS)

gdb: debug
	gdb $(PROJECT)

memcheck: debug
	valgrind --leak-check=yes ./$(PROJECT)

memcheck_v: debug
	valgrind --leak-check=yes -v ./$(PROJECT)

memcheck_full: debug
	valgrind --leak-check=full --show-leak-kinds=debug ./$(PROJECT)

clean:
	rm $(PROJECT)

install:
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	install $(PROJECT) $(DESTDIR)$(PREFIX)/bin/$(PROJECT)

uninstall:
	rm $(DESTDIR)$(PREFIX)/bin/$(PROJECT)

site:
	./$(PROJECT)

.PHONY: all debug gdb memcheck memcheck_v memcheck_full clean install uninstall site
