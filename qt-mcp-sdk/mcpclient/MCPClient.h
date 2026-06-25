#ifndef MCPCLIENT_H
#define MCPCLIENT_H

#include "mcpclient_global.h"

#include <QObject>
#include <QJsonObject>
#include <QJsonValue>
#include <QHash>
#include <QString>
#include <functional>
#include <memory>

class MCPTransport;

//
// MCPClient
//
// MCP protocol client implementation.
// Supports stdio and HTTP(s) transport modes.
//

class MCPCLIENT_EXPORT MCPClient : public QObject
{
    Q_OBJECT

public:
    // Response callback: receives the result or error
    using ResponseCallback = std::function<void(bool success, const QJsonValue &result, int errorCode, const QString &errorMessage)>;

    // Notification handler: receives method and params
    using NotificationHandler = std::function<void(const QString &method, const QJsonObject &params)>;

    explicit MCPClient(std::unique_ptr<MCPTransport> transport, QObject *parent = nullptr);
    ~MCPClient() override;

    // Lifecycle (named open/close to avoid conflict with QObject::connect)
    void openConnection();
    void closeConnection();
    bool isConnected() const;

    // Initialize handshake (MCP initialize request)
    void initialize(const QString &clientName = QStringLiteral("QtMCPClient"),
                    const QString &clientVersion = QStringLiteral("1.0.0"),
                    ResponseCallback callback = nullptr);

    // Send a JSON-RPC request
    void sendRequest(const QString &method, const QJsonObject &params,
                     ResponseCallback callback = nullptr);

    // Send a JSON-RPC notification (no response expected)
    void sendNotification(const QString &method, const QJsonObject &params = QJsonObject());

    // Send a ping request
    void ping(ResponseCallback callback = nullptr);

    // List available tools
    void listTools(ResponseCallback callback = nullptr);

    // Call a tool
    void callTool(const QString &name, const QJsonObject &arguments,
                  ResponseCallback callback = nullptr);

    // List available resources
    void listResources(ResponseCallback callback = nullptr);

    // Read a resource
    void readResource(const QString &uri, ResponseCallback callback = nullptr);

    // List available prompts
    void listPrompts(ResponseCallback callback = nullptr);

    // Get a prompt
    void getPrompt(const QString &name, const QJsonObject &arguments = QJsonObject(),
                   ResponseCallback callback = nullptr);

    // Set notification handler
    void setNotificationHandler(NotificationHandler handler);

    // Client info
    void setClientInfo(const QString &name, const QString &version);

signals:
    void connected();
    void disconnected();
    void initialized();
    void errorOccurred(const QString &errorMessage);

private slots:
    void onMessageReceived(const QJsonObject &message);
    void onTransportError(const QString &errorMessage);

private:
    QJsonValue sendRequestInternal(const QString &method, const QJsonObject &params,
                                    ResponseCallback callback);

    struct PendingRequest {
        QString method;
        ResponseCallback callback;
    };

    std::unique_ptr<MCPTransport> m_transport;
    QHash<QJsonValue, PendingRequest> m_pendingRequests;
    int m_requestCounter = 0;

    QString m_clientName = QStringLiteral("QtMCPClient");
    QString m_clientVersion = QStringLiteral("1.0.0");
    bool m_initialized = false;
    bool m_connected = false;

    NotificationHandler m_notificationHandler;
};

#endif // MCPCLIENT_H
