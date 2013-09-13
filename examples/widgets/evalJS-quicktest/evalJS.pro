QT      +=  webenginewidgets

#CONFIG += c++11
QMAKE_CXXFLAGS += -std=c++11 -fno-rtti

#include($$QTWEBENGINE_ROOT/common.pri)
include(../../../common.pri)

SOURCES =   main.cpp

