#include "MCPStdioTransport.h"

#include <QJsonDocument>
#include <QDebug>
#include <QFile>
#include <io.h>
#include <conio.h>
#include <windows.h>

// ============================================================
// Constructor / Destructor
// ============================================================

MCPStdioTransport::MCPStdioTransport(Mode mode, QObject *parent)
    : MCPTransport(parent)
    , m_mode(mode)
{
}

MCPStdioTransport::~MCPStdioTransport()
{
    close();
}

// ============================================================
// Configuration
// ============================================================

void MCPStdioTransport::setCommand(const QString &program, const QStringList &args)
{
    m_program = program;
    m_args = args;
}

void MCPStdioTransport::setWorkingDirectory(const QString &dir)
{
    m_workingDirectory = dir;
}

// ============================================================
// Transport Interface
// ============================================================

void MCPStdioTransport::open()
{
    if (m_opened) {
        qWarning() << "MCPStdioTransport: already open";
        return;
    }

    if (m_mode == Mode::Server) {
        // Server mode: read from stdin using a polling timer.
        // On Windows, QSocketNotifier does not work with console stdin (fd 0),
        // and QFile::readyRead does not fire for pipes.
        // QTimer-based polling is the reliable cross-platform fallback.
        m_stdinPollTimer = new QTimer(this);
        connect(m_stdinPollTimer, &QTimer::timeout, this, [this]() {
            // Non-blocking read from stdin (fd 0).
            // On Windows, _kbhit() only works for console input, not pipes.
            // Instead, use a non-blocking read via _read() with _O_NOWAIT
            // or simply attempt to peek at available bytes.
            // The safest cross-platform approach: use select() on Unix,
            // and attempt a small read on Windows (it will block if no data,
            // but since we use a timer, we just do a quick peek).
            HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
            if (hStdin == INVALID_HANDLE_VALUE || hStdin == nullptr)
                return;

            DWORD bytesAvail = 0;
            if (!PeekNamedPipe(hStdin, nullptr, 0, nullptr, &bytesAvail, nullptr))
                return; // no data or error

            if (bytesAvail == 0)
                return;

            QByteArray buf;
            buf.resize(static_cast<int>(bytesAvail));
            DWORD bytesRead = 0;
            if (ReadFile(hStdin, buf.data(), bytesAvail, &bytesRead, nullptr) && bytesRead > 0) {
                buf.resize(static_cast<int>(bytesRead));
                processIncomingData(buf);
            }
        });
        m_stdinPollTimer->start(10); // poll every 10ms
        qDebug() << "MCPStdioTransport: polling stdin every 10ms";

        m_opened = true;
        Q_EMIT opened();
        return;
    }

    // Client mode: spawn child process
    if (m_process) {
        qWarning() << "MCPStdioTransport: already open";
        return;
    }

    m_process = new QProcess(this);
    m_process->setProcessChannelMode(QProcess::SeparateChannels);

    connect(m_process, &QProcess::started, this, &MCPStdioTransport::onProcessStarted);
    connect(m_process, &QProcess::readyReadStandardOutput, this, &MCPStdioTransport::onReadyReadStdout);
    connect(m_process, &QProcess::readyReadStandardError, this, &MCPStdioTransport::onReadyReadStderr);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &MCPStdioTransport::onProcessFinished);

    if (!m_workingDirectory.isEmpty())
        m_process->setWorkingDirectory(m_workingDirectory);

    qDebug() << "MCPStdioTransport: starting process:" << m_program << m_args;
    m_process->start(m_program, m_args);
}

void MCPStdioTransport::close()
{
    if (!m_opened)
        return;

    m_opened = false;
    if (m_stdinPollTimer) {
        m_stdinPollTimer->stop();
        delete m_stdinPollTimer;
        m_stdinPollTimer = nullptr;
    }
    if (m_process) {
        m_process->kill();
        m_process->waitForFinished(3000);
        delete m_process;
        m_process = nullptr;
    }
    Q_EMIT closed();
}

bool MCPStdioTransport::isOpen() const
{
    return m_opened;
}

void MCPStdioTransport::sendMessage(const QJsonObject &message)
{
    QJsonDocument doc(message);
    QByteArray data = doc.toJson(QJsonDocument::Compact) + "\n";

    if (m_mode == Mode::Server) {
        // Write to stdout using low-level write for consistency with _read
        _write(1, data.constData(), static_cast<unsigned int>(data.size()));
    } else if (m_process) {
        // Write to child process stdin
        m_process->write(data);
        m_process->waitForBytesWritten(100);
    }
}

// ============================================================
// Private Slots
// ============================================================

void MCPStdioTransport::onProcessStarted()
{
    qDebug() << "MCPStdioTransport: child process started";
    m_opened = true;
    Q_EMIT processStarted();
    Q_EMIT opened();
}

void MCPStdioTransport::onReadyReadStdout()
{
    QByteArray data = m_process->readAllStandardOutput();
    processIncomingData(data);
}

void MCPStdioTransport::onReadyReadStderr()
{
    QByteArray data = m_process->readAllStandardError();
    qDebug().noquote() << "[MCP Child stderr]:" << QString::fromUtf8(data).trimmed();
}

void MCPStdioTransport::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitStatus)
    qDebug() << "MCPStdioTransport: child process finished with exit code" << exitCode;
    Q_EMIT errorOccurred(QStringLiteral("Child process exited with code %1").arg(exitCode));
    Q_EMIT closed();
}

// ============================================================
// Internal
// ============================================================

void MCPStdioTransport::processIncomingData(const QByteArray &data)
{
    m_readBuffer.append(data);

    // Process complete JSON lines (newline-delimited JSON)
    while (true) {
        int newlineIdx = m_readBuffer.indexOf('\n');
        if (newlineIdx < 0)
            break;

        QByteArray line = m_readBuffer.left(newlineIdx).trimmed();
        m_readBuffer.remove(0, newlineIdx + 1);

        if (line.isEmpty())
            continue;

        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(line, &parseError);

        if (parseError.error != QJsonParseError::NoError) {
            qWarning() << "MCPStdioTransport: JSON parse error:" << parseError.errorString();
            continue;
        }

        if (!doc.isObject()) {
            qWarning() << "MCPStdioTransport: received non-object JSON";
            continue;
        }

        Q_EMIT messageReceived(doc.object());
    }

    // Guard against unbounded buffer growth
    if (m_readBuffer.size() > m_receiveBufferSize) {
        qWarning() << "MCPStdioTransport: buffer overflow, clearing";
        m_readBuffer.clear();
    }
}
