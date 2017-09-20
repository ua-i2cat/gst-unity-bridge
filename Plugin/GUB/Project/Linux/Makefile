SHELL = /bin/sh
CC    = gcc
CXX   = g++
CXXFLAGS = -std=c++14
FLAGS        = # -std=gnu99 -Iinclude

OUTDIR = ../../Build/Linux
SRCDIR = ../../Source
PLUGINDIR = ../../../DvbCssWc

CFLAGS       = -fPIC -g  `pkg-config gstreamer-1.0 --cflags` -I/usr/local/lib/gstreamer-1.0/include -I/usr/local/include/gstreamer-1.0 -I./$(PLUGINDIR)/Source -I./Plugin/src#-pedantic -Wall -Wextra -ggdb3 
LDFLAGS      = -shared
LIBS         = $(shell pkg-config --libs glib-2.0) $(shell pkg-config --libs gio-2.0) $(shell pkg-config --libs gstreamer-1.0) -lm 
DEBUGFLAGS   = -O0 -D _DEBUG
RELEASEFLAGS = -O2 -D NDEBUG -fwhole-program

TARGET  = $(OUTDIR)/GstUnityBridge.so
SOURCES = $(shell echo $(SRCDIR)/*.c) 
OBJECTS = $(SOURCES:.c=.o)

TARGET_TEST = testGUB-Linux
TEST_OUTDIR = $(OUTDIR)/test

PREFIX = $(DESTDIR)/usr/local
BINDIR = $(PREFIX)/bin

all: $(TARGET)

$(TARGET): $(OBJECTS)
	rm -rf $(OUTDIR) && mkdir -p $(OUTDIR)
	$(CC) $(FLAGS) $(CFLAGS) $(RELEASEFLAGS) -o $(TARGET) $(LDFLAGS) $(OBJECTS) $(LIBS)

clean:
	@rm -rf src/*.o $(TARGET)

test:
	rm -rf $(TEST_OUTDIR) && mkdir -p $(TEST_OUTDIR)
	$(CXX) $(CXXFLAGS) $(CFLAGS) -I$(SRCDIR) $(DEBUGFLAGS) -L. -o $(TEST_OUTDIR)/$(TARGET_TEST) $(SRCDIR)/tests/linux-test/testGUB-Linux.c $(LIBS) -l:$(TARGET) -lgstnet-1.0 -lgstapp-1.0 -lgstvideo-1.0 -lgstgl-1.0 -lgstpbutils-1.0 -l:$(PLUGINDIR)/Build/libDvbCssWc.so -lSDL2 -l:$(OUTDIR)/GstUnityBridge.so -lGL
