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
	$(CC) $(CFLAGS) -O3 $(INCLUDES) $(LIBS) *.c -o $(PROJECT)

debug: 
	$(CC) $(CFLAGS) -g $(INCLUDES) $(LIBS) *.c -o $(PROJECT)

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
	mkdir -p $(DESTDIR)$(PREFIX)/share/doc/$(PROJECT)
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	install $(PROJECT) $(DESTDIR)$(PREFIX)/bin/$(PROJECT)
	cp -r example $(DESTDIR)$(PREFIX)/share/doc/$(PROJECT)

site:
	./$(PROJECT)
