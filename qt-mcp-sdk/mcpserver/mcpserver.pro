QT -= gui

TEMPLATE = lib
DEFINES += MCPSERVER_LIBRARY

include(../common.pri)

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

# Depend on mcpcore library
INCLUDEPATH += $$PWD/../mcpcore
LIBS += -L$$DESTDIR -lmcpcore
PRE_TARGETDEPS += $$DESTDIR/mcpcore.lib

SOURCES += \
    MCPServer.cc

HEADERS += \
    mcpserver_global.h \
    MCPServer.h

# Default rules for deployment.
unix {
    target.path = /usr/lib
}
!isEmpty(target.path): INSTALLS += target
