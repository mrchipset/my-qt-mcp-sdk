#ifndef MCPCLIENT_GLOBAL_H
#define MCPCLIENT_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(MCPCLIENT_LIBRARY)
#define MCPCLIENT_EXPORT Q_DECL_EXPORT
#else
#define MCPCLIENT_EXPORT Q_DECL_IMPORT
#endif

#endif // MCPCLIENT_GLOBAL_H
