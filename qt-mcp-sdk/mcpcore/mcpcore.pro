QT -= gui
QT += network

TEMPLATE = lib
DEFINES += MCPCORE_LIBRARY

include(../common.pri)

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    MCPCore.cc \
    MCPTypes.cc \
    MCPJsonRpcSerializer.cc \
    MCPTransport.cc \
    MCPStdioTransport.cc \
    MCPHttpTransport.cc

HEADERS += \
    mcpcore_global.h \
    MCPCore.h \
    MCPTypes.h \
    MCPJsonRpcSerializer.h \
    MCPTransport.h \
    MCPStdioTransport.h \
    MCPHttpTransport.h

# Default rules for deployment.
unix {
    target.path = /usr/lib
}
!isEmpty(target.path): INSTALLS += target
