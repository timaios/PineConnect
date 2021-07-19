CXX = g++
CXXFLAGS = -g -Wall -Wpedantic

DBUS_INCS = `pkg-config --cflags dbus-1`
DBUS_LIBS = `pkg-config --libs dbus-1`



# default target

.PHONY: all

all: \
	build/daemon/pineconnectd



# Libraries


build/lib/Logger.o: src/lib/logger/Logger.h src/lib/logger/Logger.cc
	@mkdir -p build/lib
	$(CXX) -c -o build/lib/Logger.o src/lib/logger/Logger.cc

build/lib/BluezAdapter.o: src/lib/bluez/BluezAdapter.h src/lib/bluez/BluezAdapter.cc src/lib/logger/Logger.h
	@mkdir -p build/lib
	$(CXX) $(DBUS_INCS) -Isrc/lib/logger -c -o build/lib/BluezAdapter.o src/lib/bluez/BluezAdapter.cc



# PineConnect daemon

DAEMON_HDRS = \
	src/lib/logger/Logger.h \
	src/lib/bluez/BluezAdapter.h \
	src/daemon/Device.h \
	src/daemon/ManagedDevice.h \
	src/daemon/DeviceManager.h \
	src/daemon/GattService.h \
	src/daemon/CurrentTimeService.h

DAEMON_LIB_INCS = \
	-Isrc/lib/logger \
	-Isrc/lib/bluez \
	$(DBUS_INCS)

DAEMON_OBJS = \
	build/daemon/main.o \
	build/daemon/Device.o \
	build/daemon/ManagedDevice.o \
	build/daemon/DeviceManager.o \
	build/daemon/GattService.o \
	build/daemon/CurrentTimeService.o \
	build/lib/Logger.o \
	build/lib/BluezAdapter.o

build/daemon/pineconnectd: $(DAEMON_OBJS)
	@mkdir -p build/daemon
	$(CXX) -o build/daemon/pineconnectd $(DAEMON_OBJS) $(DBUS_LIBS)

build/daemon/main.o: $(DAEMON_HDRS) src/daemon/main.cc
	@mkdir -p build/daemon
	$(CXX) $(DAEMON_LIB_INCS) -c -o build/daemon/main.o src/daemon/main.cc

build/daemon/Device.o: $(DAEMON_HDRS) src/daemon/Device.cc
	@mkdir -p build/daemon
	$(CXX) $(DAEMON_LIB_INCS) -c -o build/daemon/Device.o src/daemon/Device.cc

build/daemon/ManagedDevice.o: $(DAEMON_HDRS) src/daemon/ManagedDevice.cc
	@mkdir -p build/daemon
	$(CXX) $(DAEMON_LIB_INCS) -c -o build/daemon/ManagedDevice.o src/daemon/ManagedDevice.cc

build/daemon/DeviceManager.o: $(DAEMON_HDRS) src/daemon/DeviceManager.cc
	@mkdir -p build/daemon
	$(CXX) $(DAEMON_LIB_INCS) -c -o build/daemon/DeviceManager.o src/daemon/DeviceManager.cc

build/daemon/GattService.o: $(DAEMON_HDRS) src/daemon/GattService.cc
	@mkdir -p build/daemon
	$(CXX) $(DAEMON_LIB_INCS) -c -o build/daemon/GattService.o src/daemon/GattService.cc

build/daemon/CurrentTimeService.o: $(DAEMON_HDRS) src/daemon/CurrentTimeService.cc
	@mkdir -p build/daemon
	$(CXX) $(DAEMON_LIB_INCS) -c -o build/daemon/CurrentTimeService.o src/daemon/CurrentTimeService.cc



# clean up

.PHONY: clean

clean:
	rm -Rf build

