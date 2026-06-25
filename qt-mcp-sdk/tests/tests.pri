# tests.pri — Qt MCP SDK unit test shared configuration
# Include this in each test subproject

include($$PWD/../common.pri)

# Qt Test module
QT += testlib

# Link all SDK libraries
INCLUDEPATH += $$PWD/../mcpcore \
               $$PWD/../mcpserver \
               $$PWD/../mcpclient

LIBS += -L$$DESTDIR -lmcpcore -lmcpserver -lmcpclient
PRE_TARGETDEPS += $$DESTDIR/mcpcore.lib \
                  $$DESTDIR/mcpserver.lib \
                  $$DESTDIR/mcpclient.lib

# Console app (for test output)
CONFIG += console

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
