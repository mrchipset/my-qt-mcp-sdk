// ================================================================
// simpleclient — MCP Client Example (GUI)
//
// Provides a graphical interface connecting to an MCP server using
// stdio or HTTP transport. Allows interactive tool calls.
// ================================================================

#include "MainWindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("simpleclient"));
    app.setApplicationVersion(QStringLiteral("1.0.0"));

    MainWindow w;
    w.show();
    return app.exec();
}
