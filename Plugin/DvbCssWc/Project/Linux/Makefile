SHELL = /bin/sh
CC    = gcc
FLAGS = # -std=gnu99 -Iinclude

OUTDIR = ../../Build/Linux
SRCDIR = ../../Source

CFLAGS       = -fPIC -g `pkg-config gstreamer-1.0 --cflags-only-I` #-pedantic -Wall -Wextra -ggdb3
LDFLAGS      = -shared
LIBS         = $(shell pkg-config --libs glib-2.0) $(shell pkg-config --libs gio-2.0) $(shell pkg-config --libs gstreamer-1.0) $(shell pkg-config --libs gstreamer-net-1.0) -lm
ADDLIBS      = 
DEBUGFLAGS   = -O0 -D _DEBUG
RELEASEFLAGS = -O2 -D NDEBUG -fwhole-program

TARGET  = $(OUTDIR)/libDvbCssWc.so
SOURCES = ${wildcard $(SRCDIR)/*.c $(SRCDIR)/export/*.c}
OBJECTS = $(SOURCES:.c=.o)

PREFIX = $(DESTDIR)/usr/local
BINDIR = $(PREFIX)/bin

all: $(TARGET) examples_client examples_server

$(TARGET): $(OBJECTS)
	$(CC) $(FLAGS) $(CFLAGS) $(RELEASEFLAGS) -o $(TARGET) $(LDFLAGS) $(OBJECTS) $(LIBS)

clean:
	@rm -rf $(OBJECTS) $(TARGET) $(OUTDIR)/test_client $(OUTDIR)/test_server $(OUTDIR)/sync_test $(OUTDIR)/export_client_test

examples_client:
	$(CC) $(FLAGS) $(CFLAGS) -I$(SRCDIR) $(DEBUGFLAGS) -L. -o $(OUTDIR)/test_client $(SRCDIR)/examples/dvbcsswc-client.c $(LIBS) -l:$(TARGET) $(ADDLIBS)

examples_server:
	$(CC) $(FLAGS) $(CFLAGS) -I$(SRCDIR) $(DEBUGFLAGS) -L. -o $(OUTDIR)/test_server $(SRCDIR)/examples/dvbcsswc-server.c $(LIBS) -l:$(TARGET) $(ADDLIBS)

sync_test:
	$(CC) $(FLAGS) $(CFLAGS) -I$(SRCDIR) $(DEBUGFLAGS) -L. -o $(OUTDIR)/sync_test $(SRCDIR)/tests/dvbcsswc-sync.c $(LIBS) -l:$(TARGET) $(ADDLIBS)

export_client_test:
	$(CC) $(FLAGS) $(CFLAGS) -I$(SRCDIR) $(DEBUGFLAGS) -L. -o $(OUTDIR)/export_client_test $(SRCDIR)/tests/export-client.c $(LIBS) -l:$(TARGET) $(ADDLIBS)
