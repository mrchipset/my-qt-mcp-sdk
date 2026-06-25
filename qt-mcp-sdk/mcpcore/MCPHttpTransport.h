#ifndef MCPHTTPTRANSPORT_H
#define MCPHTTPTRANSPORT_H

#include "mcpcore_global.h"
#include "MCPTransport.h"

#include <QTcpServer>
#include <QTcpSocket>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QHash>
#include <QQueue>
#include <QByteArray>
#include <QSslConfiguration>

//
// MCPHttpTransport
//
// Implements MCP transport over HTTP(s).
//
// Modes:
//   - Server mode: listens on a TCP port for HTTP POST requests containing JSON-RPC messages
//   - Client mode: sends HTTP POST requests to a remote MCP server
//
// The transport uses JSON-RPC over HTTP POST with Content-Type: application/json.
// For server-push notifications, SSE (Server-Sent Events) can be used.
//

class MCPCORE_EXPORT MCPHttpTransport : public MCPTransport
{
    Q_OBJECT

public:
    enum class Mode {
        Server,
        Client
    };

    explicit MCPHttpTransport(Mode mode = Mode::Client, QObject *parent = nullptr);
    ~MCPHttpTransport() override;

    // Server mode: start listening on a port
    void setHost(const QString &host, quint16 port);
    void setSslConfiguration(const QSslConfiguration &sslConfig);

    // Client mode: set the server URL
    void setUrl(const QUrl &url);

    // Session management (for SSE-based server push)
    void setSessionId(const QString &sessionId);
    QString sessionId() const;

    // MCPTransport interface
    void open() override;
    void close() override;
    bool isOpen() const override;
    void sendMessage(const QJsonObject &message) override;

signals:
    // Emitted when the HTTP server starts listening (Server mode)
    void serverStarted();
    void clientConnected();

private slots:
    // Server slots
    void onNewConnection();

    // Client slots
    void onReplyFinished(QNetworkReply *reply);

private:
    void handleIncomingJson(const QByteArray &data, QTcpSocket *socket, const QString &sessionId);
    void writeHttpResponse(QTcpSocket *socket, const QByteArray &jsonBody);
    void flushPendingResponses(QTcpSocket *socket);

    Mode m_mode;

    // Server members
    QString m_host = QStringLiteral("127.0.0.1");
    quint16 m_port = 8080;
    QTcpServer *m_server = nullptr;
    // Maps pending sockets to their receive buffers
    QHash<QTcpSocket *, QByteArray> m_clientBuffers;
    // Maps pending sockets to their session ids
    QHash<QTcpSocket *, QString> m_clientSessionIds;
    // Queue of responses waiting to be written (socket -> JSON bytes)
    QHash<QTcpSocket *, QQueue<QByteArray>> m_pendingResponses;
    QSslConfiguration m_sslConfig;
    bool m_useSsl = false;

    // Client members
    QUrl m_url;
    QNetworkAccessManager *m_networkManager = nullptr;
    QString m_sessionId;

    // Incremental request id for correlation
    int m_requestCounter = 0;

    // Tracks the current socket being served during synchronous request handling
    QTcpSocket *m_currentRequestSocket = nullptr;
};

#endif // MCPHTTPTRANSPORT_H
