QT -= gui
QT += network testlib

TEMPLATE = app
TARGET = test_transport

include($$PWD/../tests.pri)

SOURCES += \
    tst_Transport.cc
