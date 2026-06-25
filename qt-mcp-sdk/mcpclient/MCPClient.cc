#include "MCPClient.h"
#include "MCPTransport.h"

#include <QJsonDocument>
#include <QDebug>

// ============================================================
// Constructor / Destructor
// ============================================================

MCPClient::MCPClient(std::unique_ptr<MCPTransport> transport, QObject *parent)
    : QObject(parent)
    , m_transport(std::move(transport))
{
    QObject::connect(m_transport.get(), &MCPTransport::messageReceived,
                     this, &MCPClient::onMessageReceived);
    QObject::connect(m_transport.get(), &MCPTransport::errorOccurred,
                     this, &MCPClient::onTransportError);
    QObject::connect(m_transport.get(), &MCPTransport::opened,
                     this, [this]() {
        m_connected = true;
        emit connected();
    });
    QObject::connect(m_transport.get(), &MCPTransport::closed,
                     this, [this]() {
        m_connected = false;
        m_initialized = false;
        m_pendingRequests.clear();
        emit disconnected();
    });
}

MCPClient::~MCPClient()
{
    closeConnection();
}

// ============================================================
// Lifecycle
// ============================================================

void MCPClient::openConnection()
{
    if (m_connected) {
        qWarning() << "MCPClient: already connected";
        return;
    }
    m_transport->open();
}

void MCPClient::closeConnection()
{
    if (m_connected) {
        m_transport->close();
    }
}

bool MCPClient::isConnected() const
{
    return m_connected;
}

// ============================================================
// Client Info
// ============================================================

void MCPClient::setClientInfo(const QString &name, const QString &version)
{
    m_clientName = name;
    m_clientVersion = version;
}

// ============================================================
// Notification Handler
// ============================================================

void MCPClient::setNotificationHandler(NotificationHandler handler)
{
    m_notificationHandler = handler;
}

// ============================================================
// Initialize
// ============================================================

void MCPClient::initialize(const QString &clientName, const QString &clientVersion,
                            ResponseCallback callback)
{
    m_clientName = clientName;
    m_clientVersion = clientVersion;

    QJsonObject params;
    params[QStringLiteral("protocolVersion")] = QStringLiteral("2024-11-05");

    QJsonObject clientInfo;
    clientInfo[QStringLiteral("name")] = m_clientName;
    clientInfo[QStringLiteral("version")] = m_clientVersion;
    params[QStringLiteral("clientInfo")] = clientInfo;

    params[QStringLiteral("capabilities")] = QJsonObject();

    sendRequestInternal(QStringLiteral("initialize"), params, callback);
}

// ============================================================
// High-level API methods
// ============================================================

void MCPClient::sendRequest(const QString &method, const QJsonObject &params,
                             ResponseCallback callback)
{
    sendRequestInternal(method, params, callback);
}

void MCPClient::sendNotification(const QString &method, const QJsonObject &params)
{
    QJsonObject notification;
    notification[QStringLiteral("jsonrpc")] = QStringLiteral("2.0");
    notification[QStringLiteral("method")] = method;
    if (!params.isEmpty())
        notification[QStringLiteral("params")] = params;
    m_transport->sendMessage(notification);
}

void MCPClient::ping(ResponseCallback callback)
{
    sendRequestInternal(QStringLiteral("ping"), QJsonObject(), callback);
}

void MCPClient::listTools(ResponseCallback callback)
{
    sendRequestInternal(QStringLiteral("tools/list"), QJsonObject(), callback);
}

void MCPClient::callTool(const QString &name, const QJsonObject &arguments,
                          ResponseCallback callback)
{
    QJsonObject params;
    params[QStringLiteral("name")] = name;
    params[QStringLiteral("arguments")] = arguments;
    sendRequestInternal(QStringLiteral("tools/call"), params, callback);
}

void MCPClient::listResources(ResponseCallback callback)
{
    sendRequestInternal(QStringLiteral("resources/list"), QJsonObject(), callback);
}

void MCPClient::readResource(const QString &uri, ResponseCallback callback)
{
    QJsonObject params;
    params[QStringLiteral("uri")] = uri;
    sendRequestInternal(QStringLiteral("resources/read"), params, callback);
}

void MCPClient::listPrompts(ResponseCallback callback)
{
    sendRequestInternal(QStringLiteral("prompts/list"), QJsonObject(), callback);
}

void MCPClient::getPrompt(const QString &name, const QJsonObject &arguments,
                           ResponseCallback callback)
{
    QJsonObject params;
    params[QStringLiteral("name")] = name;
    if (!arguments.isEmpty())
        params[QStringLiteral("arguments")] = arguments;
    sendRequestInternal(QStringLiteral("prompts/get"), params, callback);
}

// ============================================================
// Internal: send request with id tracking
// ============================================================

QJsonValue MCPClient::sendRequestInternal(const QString &method, const QJsonObject &params,
                                           ResponseCallback callback)
{
    m_requestCounter++;
    QJsonValue id = QJsonValue(m_requestCounter);

    QJsonObject request;
    request[QStringLiteral("jsonrpc")] = QStringLiteral("2.0");
    request[QStringLiteral("id")] = id;
    request[QStringLiteral("method")] = method;
    if (!params.isEmpty())
        request[QStringLiteral("params")] = params;

    if (callback) {
        PendingRequest pending;
        pending.method = method;
        pending.callback = callback;
        m_pendingRequests[id] = pending;
    }

    m_transport->sendMessage(request);
    return id;
}

// ============================================================
// Message Handling
// ============================================================

void MCPClient::onMessageReceived(const QJsonObject &message)
{
    QString jsonrpc = message.value(QStringLiteral("jsonrpc")).toString();
    if (jsonrpc != QStringLiteral("2.0")) {
        qWarning() << "MCPClient: invalid jsonrpc version:" << jsonrpc;
        return;
    }

    // Check if this is a response or notification
    QJsonValue id = message.value(QStringLiteral("id"));

    if (id.isUndefined() || id.isNull()) {
        // This is a notification (no id)
        QString method = message.value(QStringLiteral("method")).toString();
        QJsonObject params = message.value(QStringLiteral("params")).toObject();

        if (m_notificationHandler) {
            m_notificationHandler(method, params);
        }
        emit errorOccurred(QStringLiteral("Unhandled notification: ") + method);
        return;
    }

    // This is a response — find the pending request
    auto it = m_pendingRequests.find(id);
    if (it == m_pendingRequests.end()) {
        qWarning() << "MCPClient: received response for unknown request id:" << QJsonValue(id).toVariant().toString();
        return;
    }

    PendingRequest pending = it.value();
    m_pendingRequests.erase(it);

    // Handle the initialize response specially
    if (pending.method == QStringLiteral("initialize")) {
        m_initialized = true;
        emit initialized();
    }

    if (!pending.callback)
        return;

    // Check for error response
    if (message.contains(QStringLiteral("error"))) {
        QJsonObject errorObj = message.value(QStringLiteral("error")).toObject();
        int code = errorObj.value(QStringLiteral("code")).toInt();
        QString msg = errorObj.value(QStringLiteral("message")).toString();
        pending.callback(false, QJsonValue(), code, msg);
        return;
    }

    // Success response
    QJsonValue result = message.value(QStringLiteral("result"));
    pending.callback(true, result, 0, QString());
}

void MCPClient::onTransportError(const QString &errorMessage)
{
    qWarning() << "MCPClient: transport error:" << errorMessage;
    emit errorOccurred(errorMessage);
}
