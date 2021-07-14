QT=
QMAKE_LIBS=
QMAKE_LIBS_THREAD=

QMAKE_CXXFLAGS += -pg -Wpedantic
QMAKE_LFLAGS += -pg


LIBS = -L../lib/gattlib/build/dbus -lgattlib

INCLUDEPATH += ../lib/gattlib/include


SOURCES += \
        main.cc
