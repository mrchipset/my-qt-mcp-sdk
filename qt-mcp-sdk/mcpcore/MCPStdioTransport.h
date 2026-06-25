#ifndef MCPSTDIOTRANSPORT_H
#define MCPSTDIOTRANSPORT_H

#include "mcpcore_global.h"
#include "MCPTransport.h"

#include <QProcess>
#include <QFile>
#include <QTimer>
#include <QPointer>
#include <QByteArray>

//
// MCPStdioTransport
//
// Implements MCP transport over stdin/stdout.
//
// Modes:
//   - Server mode: reads from stdin, writes to stdout (classic MCP stdio mode)
//   - Client mode: spawns a child process and communicates via its stdin/stdout
//

class MCPCORE_EXPORT MCPStdioTransport : public MCPTransport
{
    Q_OBJECT

public:
    enum class Mode {
        Server,  // Read from process stdin/stdout
        Client   // Spawn child process
    };

    explicit MCPStdioTransport(Mode mode = Mode::Client, QObject *parent = nullptr);
    ~MCPStdioTransport() override;

    // Configure the child process to launch (Client mode)
    void setCommand(const QString &program, const QStringList &args = {});

    // Configure working directory for child process (Client mode)
    void setWorkingDirectory(const QString &dir);

    // MCPTransport interface
    void open() override;
    void close() override;
    bool isOpen() const override;
    void sendMessage(const QJsonObject &message) override;

signals:
    // Emitted when the child process has started (Client mode)
    void processStarted();

private slots:
    void onReadyReadStdout();
    void onReadyReadStderr();
    void onProcessStarted();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    void processIncomingData(const QByteArray &data);

    Mode m_mode;
    QPointer<QProcess> m_process;
    QTimer *m_stdinPollTimer = nullptr;
    QString m_program;
    QStringList m_args;
    QString m_workingDirectory;
    QByteArray m_readBuffer;
    bool m_opened = false;
};

#endif // MCPSTDIOTRANSPORT_H
