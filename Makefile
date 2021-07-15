CXX=g++
CXXFLAGS=-g -Wall -Wpedantic


# PineConnect daemon

build/daemon/pc-daemon: build/daemon/main.o
	$(CXX) -Llib/gattlib/build/dbus -o build/daemon/pc-daemon build/daemon/main.o -lgattlib

build/daemon/main.o: src/daemon/main.cc
	mkdir -p build/daemon
	$(CXX) -Ilib/gattlib/include -c -o build/daemon/main.o src/daemon/main.cc


# clean up

.PHONY: clean

clean:
	rm -Rf build

