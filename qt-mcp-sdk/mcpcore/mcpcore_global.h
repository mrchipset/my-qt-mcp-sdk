#ifndef MCPCORE_GLOBAL_H
#define MCPCORE_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(MCPCORE_LIBRARY)
#define MCPCORE_EXPORT Q_DECL_EXPORT
#else
#define MCPCORE_EXPORT Q_DECL_IMPORT
#endif

#endif // MCPCORE_GLOBAL_H
