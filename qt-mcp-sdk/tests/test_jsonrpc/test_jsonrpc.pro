QT -= gui

TEMPLATE = app
TARGET = test_jsonrpc

include($$PWD/../tests.pri)

SOURCES += \
    tst_JsonRpcSerializer.cc
