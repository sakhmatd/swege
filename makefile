CC = cc

CFLAGS  = -std=c99
CFLAGS += -g
CFLAGS += -pedantic
CFLAGS += -Werror
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

memcheck:
	valgrind --leak-check=yes ./$(PROJECT)

memcheck_v:
	valgrind --leak-check=yes -v ./$(PROJECT)

clean:
	rm $(PROJECT)

install:
	cp $(PROJECT) $(DESTDIR)/$(PREFIX)/bin/$(PROJECT)
