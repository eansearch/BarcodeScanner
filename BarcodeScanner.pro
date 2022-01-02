QT       += core
QT       += network
QT       += gui widgets

TARGET = BarcodeScanner

TEMPLATE = app

SOURCES += main.cpp BarcodeScanner.cpp
HEADERS += BarcodeScanner.h
unix:LIBS += `pkg-config --cflags --libs gstreamer-1.0 gstreamer-video-1.0`
unix:QMAKE_CXXFLAGS += -g -O0 `pkg-config --cflags --libs gstreamer-1.0`

target.path += /usr/local/bin
INSTALLS += target

cppcheck.commands = cppcheck --enable=all $(INCLUDEPATH) `pkg-config --cflags-only-I gstreamer-1.0` main.cpp BarcodeScanner.cpp
QMAKE_EXTRA_TARGETS += cppcheck

