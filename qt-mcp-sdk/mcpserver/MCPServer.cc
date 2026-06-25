#include "MCPServer.h"
#include "MCPTransport.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QDebug>

// ============================================================
// Constructor / Destructor
// ============================================================

MCPServer::MCPServer(std::unique_ptr<MCPTransport> transport, QObject *parent)
    : QObject(parent)
    , m_transport(std::move(transport))
{
    QObject::connect(m_transport.get(), &MCPTransport::messageReceived,
                     this, &MCPServer::onMessageReceived);
    QObject::connect(m_transport.get(), &MCPTransport::errorOccurred,
                     this, &MCPServer::onTransportError);
    QObject::connect(m_transport.get(), &MCPTransport::opened,
                     this, [this]() {
        m_running = true;
        emit started();
    });
    QObject::connect(m_transport.get(), &MCPTransport::closed,
                     this, [this]() {
        m_running = false;
        m_initialized = false;
        emit stopped();
    });
}

MCPServer::~MCPServer()
{
    stop();
}

// ============================================================
// Lifecycle
// ============================================================

void MCPServer::start()
{
    if (m_running) {
        qWarning() << "MCPServer: already running";
        return;
    }
    m_transport->open();
}

void MCPServer::stop()
{
    if (m_running) {
        m_transport->close();
    }
}

bool MCPServer::isRunning() const
{
    return m_running;
}

// ============================================================
// Tool Registration
// ============================================================

void MCPServer::registerTool(const QString &name, const QString &description,
                              const QJsonObject &inputSchema, ToolHandler handler)
{
    ToolEntry entry;
    entry.name = name;
    entry.description = description;
    entry.inputSchema = inputSchema;
    entry.handler = handler;
    m_tools[name] = entry;
}

void MCPServer::removeTool(const QString &name)
{
    m_tools.remove(name);
}

bool MCPServer::hasTool(const QString &name) const
{
    return m_tools.contains(name);
}

QStringList MCPServer::toolNames() const
{
    return m_tools.keys();
}

// ============================================================
// Resource Registration
// ============================================================

void MCPServer::registerResource(const QString &uri, const QString &name,
                                  const QString &description, const QString &mimeType,
                                  ResourceReader reader)
{
    ResourceEntry entry;
    entry.uri = uri;
    entry.name = name;
    entry.description = description;
    entry.mimeType = mimeType;
    entry.reader = reader;
    m_resources[uri] = entry;
}

void MCPServer::removeResource(const QString &uri)
{
    m_resources.remove(uri);
}

bool MCPServer::hasResource(const QString &uri) const
{
    return m_resources.contains(uri);
}

// ============================================================
// Prompt Registration
// ============================================================

void MCPServer::registerPrompt(const QString &name, const QString &description,
                                const QJsonObject &argumentsSchema, PromptHandler handler)
{
    PromptEntry entry;
    entry.name = name;
    entry.description = description;
    entry.argumentsSchema = argumentsSchema;
    entry.handler = handler;
    m_prompts[name] = entry;
}

void MCPServer::removePrompt(const QString &name)
{
    m_prompts.remove(name);
}

bool MCPServer::hasPrompt(const QString &name) const
{
    return m_prompts.contains(name);
}

// ============================================================
// Server Info
// ============================================================

void MCPServer::setServerInfo(const QString &name, const QString &version)
{
    m_serverName = name;
    m_serverVersion = version;
}

void MCPServer::setServerCapabilities(bool tools, bool resources, bool prompts, bool logging)
{
    m_capTools = tools;
    m_capResources = resources;
    m_capPrompts = prompts;
    m_capLogging = logging;
}

// ============================================================
// Message Handling
// ============================================================

void MCPServer::onMessageReceived(const QJsonObject &message)
{
    QString jsonrpc = message.value(QStringLiteral("jsonrpc")).toString();
    if (jsonrpc != QStringLiteral("2.0")) {
        qWarning() << "MCPServer: invalid jsonrpc version:" << jsonrpc;
        return;
    }

    QJsonValue id = message.value(QStringLiteral("id"));
    QString method = message.value(QStringLiteral("method")).toString();

    if (method.isEmpty() && id.isUndefined()) {
        qWarning() << "MCPServer: received message without method or id";
        return;
    }

    // Handle notifications (messages without id)
    bool isNotification = id.isUndefined() || id.isNull();
    if (isNotification) {
        if (method == QStringLiteral("notifications/initialized")) {
            qDebug() << "MCPServer: client initialized";
        } else if (method == QStringLiteral("notifications/cancelled")) {
            qDebug() << "MCPServer: request cancelled";
        }
        return;
    }

    // Handle requests
    if (method == QStringLiteral("initialize"))
        handleInitialize(message, id);
    else if (method == QStringLiteral("tools/list"))
        handleListTools(message, id);
    else if (method == QStringLiteral("tools/call"))
        handleCallTool(message, id);
    else if (method == QStringLiteral("resources/list"))
        handleListResources(message, id);
    else if (method == QStringLiteral("resources/read"))
        handleReadResource(message, id);
    else if (method == QStringLiteral("prompts/list"))
        handleListPrompts(message, id);
    else if (method == QStringLiteral("prompts/get"))
        handleGetPrompt(message, id);
    else if (method == QStringLiteral("ping"))
        handlePing(message, id);
    else
        sendErrorResponse(id, -32601, QStringLiteral("Method not found: ") + method);
}

void MCPServer::onTransportError(const QString &errorMessage)
{
    qWarning() << "MCPServer: transport error:" << errorMessage;
    emit errorOccurred(errorMessage);
}

// ============================================================
// Request Handlers
// ============================================================

void MCPServer::handleInitialize(const QJsonObject &message, const QJsonValue &id)
{
    Q_UNUSED(message)
    m_initialized = true;

    QJsonObject result;
    result[QStringLiteral("protocolVersion")] = QStringLiteral("2024-11-05");

    QJsonObject serverInfo;
    serverInfo[QStringLiteral("name")] = m_serverName;
    serverInfo[QStringLiteral("version")] = m_serverVersion;
    result[QStringLiteral("serverInfo")] = serverInfo;

    QJsonObject capabilities;
    if (m_capTools) {
        QJsonObject toolsCap;
        toolsCap[QStringLiteral("listChanged")] = false;
        capabilities[QStringLiteral("tools")] = toolsCap;
    }
    if (m_capResources) {
        QJsonObject resourcesCap;
        resourcesCap[QStringLiteral("subscribe")] = false;
        resourcesCap[QStringLiteral("listChanged")] = false;
        capabilities[QStringLiteral("resources")] = resourcesCap;
    }
    if (m_capPrompts) {
        QJsonObject promptsCap;
        promptsCap[QStringLiteral("listChanged")] = false;
        capabilities[QStringLiteral("prompts")] = promptsCap;
    }
    if (m_capLogging)
        capabilities[QStringLiteral("logging")] = QJsonObject();
    result[QStringLiteral("capabilities")] = capabilities;

    sendResponse(id, result);
}

void MCPServer::handleListTools(const QJsonObject &message, const QJsonValue &id)
{
    Q_UNUSED(message)
    QJsonObject result;
    QJsonArray toolsArray;
    for (const auto &entry : m_tools) {
        QJsonObject toolObj;
        toolObj[QStringLiteral("name")] = entry.name;
        toolObj[QStringLiteral("description")] = entry.description;
        if (!entry.inputSchema.isEmpty())
            toolObj[QStringLiteral("inputSchema")] = entry.inputSchema;
        toolsArray.append(toolObj);
    }
    result[QStringLiteral("tools")] = toolsArray;
    sendResponse(id, result);
}

void MCPServer::handleCallTool(const QJsonObject &message, const QJsonValue &id)
{
    QJsonObject params = message.value(QStringLiteral("params")).toObject();
    QString toolName = params.value(QStringLiteral("name")).toString();
    QJsonObject arguments = params.value(QStringLiteral("arguments")).toObject();

    auto it = m_tools.find(toolName);
    if (it == m_tools.end()) {
        sendErrorResponse(id, -32602, QStringLiteral("Tool not found: ") + toolName);
        return;
    }

    emit toolCalled(toolName, arguments);

    try {
        QJsonObject result = it.value().handler(arguments);

        QJsonObject responseObj;
        QJsonArray content;
        QJsonObject textContent;
        textContent[QStringLiteral("type")] = QStringLiteral("text");
        textContent[QStringLiteral("text")] = QString::fromUtf8(QJsonDocument(result).toJson(QJsonDocument::Compact));
        content.append(textContent);
        responseObj[QStringLiteral("content")] = content;

        // Check if the result has an isError flag
        if (result.contains(QStringLiteral("isError")) && result[QStringLiteral("isError")].toBool())
            responseObj[QStringLiteral("isError")] = true;

        sendResponse(id, responseObj);
    } catch (const std::exception &e) {
        sendErrorResponse(id, -32603, QStringLiteral("Tool execution error: ") + QString::fromUtf8(e.what()));
    }
}

void MCPServer::handleListResources(const QJsonObject &message, const QJsonValue &id)
{
    Q_UNUSED(message)
    QJsonObject result;
    QJsonArray resourcesArray;
    for (const auto &entry : m_resources) {
        QJsonObject resObj;
        resObj[QStringLiteral("uri")] = entry.uri;
        resObj[QStringLiteral("name")] = entry.name;
        if (!entry.description.isEmpty())
            resObj[QStringLiteral("description")] = entry.description;
        if (!entry.mimeType.isEmpty())
            resObj[QStringLiteral("mimeType")] = entry.mimeType;
        resourcesArray.append(resObj);
    }
    result[QStringLiteral("resources")] = resourcesArray;
    sendResponse(id, result);
}

void MCPServer::handleReadResource(const QJsonObject &message, const QJsonValue &id)
{
    QJsonObject params = message.value(QStringLiteral("params")).toObject();
    QString uri = params.value(QStringLiteral("uri")).toString();

    auto it = m_resources.find(uri);
    if (it == m_resources.end()) {
        sendErrorResponse(id, -32602, QStringLiteral("Resource not found: ") + uri);
        return;
    }

    try {
        QJsonObject content = it.value().reader(uri);

        QJsonObject result;
        QJsonArray contents;
        contents.append(content);
        result[QStringLiteral("contents")] = contents;

        sendResponse(id, result);
    } catch (const std::exception &e) {
        sendErrorResponse(id, -32603, QStringLiteral("Resource read error: ") + QString::fromUtf8(e.what()));
    }
}

void MCPServer::handleListPrompts(const QJsonObject &message, const QJsonValue &id)
{
    Q_UNUSED(message)
    QJsonObject result;
    QJsonArray promptsArray;
    for (const auto &entry : m_prompts) {
        QJsonObject promptObj;
        promptObj[QStringLiteral("name")] = entry.name;
        if (!entry.description.isEmpty())
            promptObj[QStringLiteral("description")] = entry.description;
        if (!entry.argumentsSchema.isEmpty())
            promptObj[QStringLiteral("arguments")] = entry.argumentsSchema.value(QStringLiteral("arguments")).toArray();
        promptsArray.append(promptObj);
    }
    result[QStringLiteral("prompts")] = promptsArray;
    sendResponse(id, result);
}

void MCPServer::handleGetPrompt(const QJsonObject &message, const QJsonValue &id)
{
    QJsonObject params = message.value(QStringLiteral("params")).toObject();
    QString promptName = params.value(QStringLiteral("name")).toString();
    QJsonObject arguments = params.value(QStringLiteral("arguments")).toObject();

    auto it = m_prompts.find(promptName);
    if (it == m_prompts.end()) {
        sendErrorResponse(id, -32602, QStringLiteral("Prompt not found: ") + promptName);
        return;
    }

    try {
        QJsonObject result = it.value().handler(arguments);
        sendResponse(id, result);
    } catch (const std::exception &e) {
        sendErrorResponse(id, -32603, QStringLiteral("Prompt execution error: ") + QString::fromUtf8(e.what()));
    }
}

void MCPServer::handlePing(const QJsonObject &message, const QJsonValue &id)
{
    Q_UNUSED(message)
    sendResponse(id, QJsonObject());
}

// ============================================================
// Response Helpers
// ============================================================

void MCPServer::sendResponse(const QJsonValue &id, const QJsonValue &result)
{
    QJsonObject response;
    response[QStringLiteral("jsonrpc")] = QStringLiteral("2.0");
    response[QStringLiteral("id")] = id;
    response[QStringLiteral("result")] = result;
    m_transport->sendMessage(response);
}

void MCPServer::sendErrorResponse(const QJsonValue &id, int code, const QString &message, const QJsonValue &data)
{
    QJsonObject response;
    response[QStringLiteral("jsonrpc")] = QStringLiteral("2.0");
    response[QStringLiteral("id")] = id;
    QJsonObject error;
    error[QStringLiteral("code")] = code;
    error[QStringLiteral("message")] = message;
    if (!data.isUndefined() && !data.isNull())
        error[QStringLiteral("data")] = data;
    response[QStringLiteral("error")] = error;
    m_transport->sendMessage(response);
}
