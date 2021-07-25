QT=
QMAKE_LIBS=
QMAKE_LIBS_THREAD=

QMAKE_CXXFLAGS += -pg -Wpedantic
QMAKE_LFLAGS += -pg


unix {
        CONFIG += link_pkgconfig
        PKGCONFIG += dbus-1
}


INCLUDEPATH += ../lib/logger ../lib/dbus


SOURCES += \
        ../lib/logger/Logger.cc \
        ../lib/dbus/BluezAdapter.cc \
        CurrentTimeService.cc \
        Device.cc \
        DeviceManager.cc \
        GattService.cc \
        ManagedDevice.cc \
        main.cc

HEADERS += \
        ../lib/logger/Logger.h \
        ../lib/dbus/BluezAdapter.h \
        CurrentTimeService.h \
        Device.h \
        DeviceManager.h \
        GattService.h \
        ManagedDevice.h
