QT -= gui
QT += network testlib

TEMPLATE = app
TARGET = test_client

include($$PWD/../tests.pri)

SOURCES += \
    tst_MCPClient.cc
