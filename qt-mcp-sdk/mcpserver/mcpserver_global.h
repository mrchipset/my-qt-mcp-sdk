#ifndef MCPSERVER_GLOBAL_H
#define MCPSERVER_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(MCPSERVER_LIBRARY)
#define MCPSERVER_EXPORT Q_DECL_EXPORT
#else
#define MCPSERVER_EXPORT Q_DECL_IMPORT
#endif

#endif // MCPSERVER_GLOBAL_H
