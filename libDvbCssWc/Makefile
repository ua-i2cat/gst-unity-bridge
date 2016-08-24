SHELL = /bin/sh
CC    = gcc
FLAGS        = # -std=gnu99 -Iinclude

CFLAGS       = -fPIC -g `pkg-config gstreamer-1.0 --cflags-only-I` #-pedantic -Wall -Wextra -ggdb3 
LDFLAGS      = -shared
LIBS         = $(shell pkg-config --libs glib-2.0) $(shell pkg-config --libs gio-2.0) $(shell pkg-config --libs gstreamer-1.0) -lm 
DEBUGFLAGS   = -O0 -D _DEBUG
RELEASEFLAGS = -O2 -D NDEBUG -fwhole-program

TARGET  = libDvbCssWc.so
SOURCES = $(shell echo src/*.c)
OBJECTS = $(SOURCES:.c=.o)

PREFIX = $(DESTDIR)/usr/local
BINDIR = $(PREFIX)/bin

all: $(TARGET) examples_client examples_server

$(TARGET): $(OBJECTS)
	$(CC) $(FLAGS) $(CFLAGS) $(RELEASEFLAGS) -o $(TARGET) $(LDFLAGS) $(OBJECTS) $(LIBS)

clean:
	@rm -rf src/*.o $(TARGET)

examples_client:
	$(CC) $(FLAGS) $(CFLAGS) -Isrc $(DEBUGFLAGS) -L. -o test_client examples/dvbcsswc-client.c $(LIBS) -l:$(TARGET)

examples_server:
	$(CC) $(FLAGS) $(CFLAGS) -Isrc $(DEBUGFLAGS) -L. -o test_server examples/dvbcsswc-server.c $(LIBS) -l:$(TARGET)

