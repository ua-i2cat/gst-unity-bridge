SHELL = /bin/sh
CC    = gcc
CXX   = g++
CXXFLAGS = -std=c++14
FLAGS        = # -std=gnu99 -Iinclude

CFLAGS       = -fPIC -g  `pkg-config gstreamer-1.0 --cflags` `pkg-config opencv --cflags` -I/usr/local/lib/gstreamer-1.0/include -I/usr/local/include/gstreamer-1.0 -I./libDvbCssWc/src -I./Plugin/src#-pedantic -Wall -Wextra -ggdb3 
LDFLAGS      = -shared
LIBS         = $(shell pkg-config --libs glib-2.0) $(shell pkg-config --libs gio-2.0) $(shell pkg-config --libs gstreamer-1.0) -lm 
DEBUGFLAGS   = -O0 -D _DEBUG
RELEASEFLAGS = -O2 -D NDEBUG -fwhole-program

TARGET  = GstUnityBridge.so
SOURCES = $(shell echo Plugin/src/*.c) 
OBJECTS = $(SOURCES:.c=.o)

PREFIX = $(DESTDIR)/usr/local
BINDIR = $(PREFIX)/bin

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(FLAGS) $(CFLAGS) $(RELEASEFLAGS) -o $(TARGET) $(LDFLAGS) $(OBJECTS) $(LIBS)

clean:
	@rm -rf src/*.o $(TARGET)

test:
	$(CXX) $(CXXFLAGS) $(CFLAGS) -Isrc $(DEBUGFLAGS) -L. -o testGUB testGUB.c $(LIBS) -l:$(TARGET) -lgstnet-1.0 -lgstapp-1.0 -lgstvideo-1.0 -lgstgl-1.0 -lgstpbutils-1.0 -l:libDvbCssWc.so -lSDL2 -l:GstUnityBridge.so 



