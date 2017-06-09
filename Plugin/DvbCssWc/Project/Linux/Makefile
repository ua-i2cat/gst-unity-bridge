SHELL = /bin/sh
CC    = gcc
FLAGS        = # -std=gnu99 -Iinclude

OUTDIR = ../../Build
SRCDIR = ../../Source

CFLAGS       = -fPIC -g `pkg-config gstreamer-1.0 --cflags-only-I` #-pedantic -Wall -Wextra -ggdb3 
LDFLAGS      = -shared
LIBS         = $(shell pkg-config --libs glib-2.0) $(shell pkg-config --libs gio-2.0) $(shell pkg-config --libs gstreamer-1.0) -lm 
DEBUGFLAGS   = -O0 -D _DEBUG
RELEASEFLAGS = -O2 -D NDEBUG -fwhole-program

TARGET  = $(OUTDIR)/libDvbCssWc.so
SOURCES = $(shell echo $(SRCDIR)/*.c)
OBJECTS = $(SOURCES:.c=.o)

PREFIX = $(DESTDIR)/usr/local
BINDIR = $(PREFIX)/bin

all: $(TARGET) examples_client examples_server

$(TARGET): $(OBJECTS)
	$(CC) $(FLAGS) $(CFLAGS) $(RELEASEFLAGS) -o $(TARGET) $(LDFLAGS) $(OBJECTS) $(LIBS)

clean:
	@rm -rf $(SRCDIR)/*.o $(TARGET) $(OUTDIR)/test_client $(OUTDIR)/test_server

examples_client:
	$(CC) $(FLAGS) $(CFLAGS) -I$(SRCDIR) $(DEBUGFLAGS) -L. -o $(OUTDIR)/test_client $(SRCDIR)/examples/dvbcsswc-client.c $(LIBS) -l:$(TARGET) $(shell pkg-config --libs gstreamer-net-1.0)

examples_server:
	$(CC) $(FLAGS) $(CFLAGS) -I$(SRCDIR) $(DEBUGFLAGS) -L. -o $(OUTDIR)/test_server $(SRCDIR)/examples/dvbcsswc-server.c $(LIBS) -l:$(TARGET) $(shell pkg-config --libs gstreamer-net-1.0)

