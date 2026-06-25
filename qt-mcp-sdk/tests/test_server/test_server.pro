QT -= gui
QT += network testlib

TEMPLATE = app
TARGET = test_server

include($$PWD/../tests.pri)

SOURCES += \
    tst_MCPServer.cc
