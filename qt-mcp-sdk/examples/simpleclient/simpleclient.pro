QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TEMPLATE = app

include(../../common.pri)

# Depend on all SDK libraries
INCLUDEPATH += $$PWD/../../mcpclient \
               $$PWD/../../mcpserver \
               $$PWD/../../mcpcore
LIBS += -L$$DESTDIR -lmcpcore -lmcpclient
PRE_TARGETDEPS += $$DESTDIR/mcpcore.lib \
                  $$DESTDIR/mcpclient.lib

SOURCES += \
    main.cpp \
    MainWindow.cc

HEADERS += \
    MainWindow.h

FORMS += \
    MainWindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
