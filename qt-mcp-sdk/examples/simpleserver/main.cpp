#include "MainWindow.h"

#include <QApplication>
#include <QCommandLineParser>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("simpleserver"));
    app.setApplicationVersion(QStringLiteral("1.0.0"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Qt MCP Simple Server"));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption stdioOpt(QStringLiteral("stdio"),
        QStringLiteral("Start in stdio mode immediately (no GUI interaction needed). "
                       "The server reads JSON-RPC from stdin and writes to stdout."));
    parser.addOption(stdioOpt);

    parser.process(app);

    bool autoStart = parser.isSet(stdioOpt);
    MainWindow w(nullptr, autoStart);
    if (!autoStart) {
        w.show(); // Only show the window in interactive mode
    }
    // In --stdio mode: window hidden, server auto-starts on stdin/stdout
    return app.exec();
}
