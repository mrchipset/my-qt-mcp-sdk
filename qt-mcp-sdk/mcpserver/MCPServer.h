#ifndef MCPSERVER_H
#define MCPSERVER_H

#include "mcpserver_global.h"

#include <QObject>
#include <QJsonObject>
#include <QJsonValue>
#include <QHash>
#include <QString>
#include <functional>
#include <memory>

// Forward declaration for QPointer guard in async callbacks
#include <QPointer>

class MCPTransport;

//
// MCPServer
//
// MCP protocol server implementation.
// Supports stdio and HTTP(s) transport modes.
//

class MCPSERVER_EXPORT MCPServer : public QObject
{
    Q_OBJECT

public:
    // Tool handler: receives params, returns result as QJsonObject
    using ToolHandler = std::function<QJsonObject(const QJsonObject &params)>;

    // Async tool handler: receives params + a "done" callback.
    // The handler should invoke done(result) when the async operation completes.
    // The callback must be called from the main thread (Qt event loop thread).
    using AsyncToolHandler = std::function<void(const QJsonObject &params,
                                                std::function<void(QJsonObject)> done)>;

    // Resource reader: receives URI, returns resource content as QJsonObject
    using ResourceReader = std::function<QJsonObject(const QString &uri)>;

    // Prompt handler: receives arguments, returns result as QJsonObject
    using PromptHandler = std::function<QJsonObject(const QJsonObject &arguments)>;

    explicit MCPServer(std::unique_ptr<MCPTransport> transport, QObject *parent = nullptr);
    ~MCPServer() override;

    // Lifecycle
    void start();
    void stop();
    bool isRunning() const;

    // Tool registration
    void registerTool(const QString &name, const QString &description,
                      const QJsonObject &inputSchema, ToolHandler handler);

    // Async tool registration — handler receives a done callback instead of returning
    void registerAsyncTool(const QString &name, const QString &description,
                           const QJsonObject &inputSchema, AsyncToolHandler handler);

    void removeTool(const QString &name);
    bool hasTool(const QString &name) const;
    bool isAsyncTool(const QString &name) const;
    QStringList toolNames() const;

    // Resource registration
    void registerResource(const QString &uri, const QString &name,
                          const QString &description, const QString &mimeType,
                          ResourceReader reader);
    void removeResource(const QString &uri);
    bool hasResource(const QString &uri) const;

    // Prompt registration
    void registerPrompt(const QString &name, const QString &description,
                        const QJsonObject &argumentsSchema, PromptHandler handler);
    void removePrompt(const QString &name);
    bool hasPrompt(const QString &name) const;

    // Server info
    void setServerInfo(const QString &name, const QString &version);
    void setServerCapabilities(bool tools, bool resources, bool prompts, bool logging);

signals:
    void started();
    void stopped();
    void errorOccurred(const QString &errorMessage);
    void clientConnected();
    void clientDisconnected();

    // Emitted when a tool is called (for monitoring/logging)
    void toolCalled(const QString &name, const QJsonObject &params);

private slots:
    void onMessageReceived(const QJsonObject &message);
    void onTransportError(const QString &errorMessage);

private:
    void handleInitialize(const QJsonObject &message, const QJsonValue &id);
    void handleListTools(const QJsonObject &message, const QJsonValue &id);
    void handleCallTool(const QJsonObject &message, const QJsonValue &id);
    void handleListResources(const QJsonObject &message, const QJsonValue &id);
    void handleReadResource(const QJsonObject &message, const QJsonValue &id);
    void handleListPrompts(const QJsonObject &message, const QJsonValue &id);
    void handleGetPrompt(const QJsonObject &message, const QJsonValue &id);
    void handlePing(const QJsonObject &message, const QJsonValue &id);

    void sendResponse(const QJsonValue &id, const QJsonValue &result);
    void sendErrorResponse(const QJsonValue &id, int code, const QString &message, const QJsonValue &data = QJsonValue());

    struct ToolEntry {
        QString name;
        QString description;
        QJsonObject inputSchema;
        ToolHandler handler;
        AsyncToolHandler asyncHandler;
        bool isAsync = false;
    };

    struct ResourceEntry {
        QString uri;
        QString name;
        QString description;
        QString mimeType;
        ResourceReader reader;
    };

    struct PromptEntry {
        QString name;
        QString description;
        QJsonObject argumentsSchema;
        PromptHandler handler;
    };

    std::unique_ptr<MCPTransport> m_transport;
    QHash<QString, ToolEntry> m_tools;
    QHash<QString, ResourceEntry> m_resources;
    QHash<QString, PromptEntry> m_prompts;

    QString m_serverName = QStringLiteral("QtMCPServer");
    QString m_serverVersion = QStringLiteral("1.0.0");
    bool m_capTools = true;
    bool m_capResources = false;
    bool m_capPrompts = false;
    bool m_capLogging = false;

    bool m_initialized = false;
    bool m_running = false;
};

#endif // MCPSERVER_H
