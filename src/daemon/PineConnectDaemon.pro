QT=
QMAKE_LIBS=
QMAKE_LIBS_THREAD=

QMAKE_CXXFLAGS += -pg -Wpedantic
QMAKE_LFLAGS += -pg


LIBS = -L../lib/gattlib/build/dbus -lgattlib -lpthread

INCLUDEPATH += ../lib/gattlib/include ../lib/logger


SOURCES += \
        ../lib/logger/Logger.cc \
        CurrentTimeService.cc \
        Device.cc \
        DeviceManager.cc \
        GattService.cc \
        ManagedDevice.cc \
        main.cc

HEADERS += \
        ../lib/logger/Logger.h \
        CurrentTimeService.h \
        Device.h \
        DeviceManager.h \
        GattService.h \
        ManagedDevice.h
