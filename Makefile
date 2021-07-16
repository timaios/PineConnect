CXX=g++
CXXFLAGS=-g -Wall -Wpedantic



# default target

.PHONY: all

all: \
	build/daemon/pineconnectd



# Libraries

build/lib/Logger.o: src/lib/logger/Logger.h src/lib/logger/Logger.cc
	@mkdir -p build/lib
	$(CXX) -c -o build/lib/Logger.o src/lib/logger/Logger.cc



# PineConnect daemon

DAEMON_HDRS = \
	src/daemon/Device.h \
	src/daemon/ManagedDevice.h \
	src/daemon/DeviceManager.h \
	src/daemon/GattService.h \
	src/daemon/CurrentTimeService.h

DAEMON_OBJS = \
	build/daemon/main.o \
	build/daemon/Device.o \
	build/daemon/ManagedDevice.o \
	build/daemon/DeviceManager.o \
	build/daemon/GattService.o \
	build/daemon/CurrentTimeService.o \
	build/lib/Logger.o

build/daemon/pineconnectd: $(DAEMON_OBJS)
	@mkdir -p build/daemon
	$(CXX) -Llib/gattlib/build/dbus -o build/daemon/pineconnectd $(DAEMON_OBJS) -lgattlib

build/daemon/main.o: src/daemon/main.cc src/lib/logger/Logger.h
	@mkdir -p build/daemon
	$(CXX) -Isrc/lib/logger -Ilib/gattlib/include -c -o build/daemon/main.o src/daemon/main.cc

build/daemon/Device.o: $(DAEMON_HDRS) src/daemon/Device.cc src/lib/logger/Logger.h
	@mkdir -p build/daemon
	$(CXX) -Isrc/lib/logger -Ilib/gattlib/include -c -o build/daemon/Device.o src/daemon/Device.cc

build/daemon/ManagedDevice.o: $(DAEMON_HDRS) src/daemon/ManagedDevice.cc src/lib/logger/Logger.h
	@mkdir -p build/daemon
	$(CXX) -Isrc/lib/logger -Ilib/gattlib/include -c -o build/daemon/ManagedDevice.o src/daemon/ManagedDevice.cc

build/daemon/DeviceManager.o: $(DAEMON_HDRS) src/daemon/DeviceManager.cc src/lib/logger/Logger.h
	@mkdir -p build/daemon
	$(CXX) -Isrc/lib/logger -Ilib/gattlib/include -c -o build/daemon/DeviceManager.o src/daemon/DeviceManager.cc

build/daemon/GattService.o: $(DAEMON_HDRS) src/daemon/GattService.cc
	@mkdir -p build/daemon
	$(CXX) -Isrc/lib/logger -Ilib/gattlib/include -c -o build/daemon/GattService.o src/daemon/GattService.cc

build/daemon/CurrentTimeService.o: $(DAEMON_HDRS) src/daemon/CurrentTimeService.cc src/lib/logger/Logger.h
	@mkdir -p build/daemon
	$(CXX) -Isrc/lib/logger -Ilib/gattlib/include -c -o build/daemon/CurrentTimeService.o src/daemon/CurrentTimeService.cc



# clean up

.PHONY: clean

clean:
	rm -Rf build

