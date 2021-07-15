CXX=g++
CXXFLAGS=-g -Wall -Wpedantic



# default target

.PHONY: all

all: \
	build/daemon/pineconnectd



# Libraries

build/lib/Logger.o: src/lib/logger/Logger.h src/lib/logger/Logger.cc
	mkdir -p build/lib
	$(CXX) -c -o build/lib/Logger.o src/lib/logger/Logger.cc



# PineConnect daemon

DAEMON_OBJS = \
	build/daemon/main.o \
	build/lib/Logger.o

build/daemon/pineconnectd: $(DAEMON_OBJS)
	mkdir -p build/daemon
	$(CXX) -Llib/gattlib/build/dbus -o build/daemon/pineconnectd $(DAEMON_OBJS) -lgattlib

build/daemon/main.o: src/daemon/main.cc src/lib/logger/Logger.h
	mkdir -p build/daemon
	$(CXX) -Isrc/lib/logger -Ilib/gattlib/include -c -o build/daemon/main.o src/daemon/main.cc



# clean up

.PHONY: clean

clean:
	rm -Rf build

