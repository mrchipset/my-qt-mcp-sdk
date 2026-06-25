#ifndef MCPTYPES_H
#define MCPTYPES_H

#include "mcpcore_global.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QString>
#include <QHash>
#include <QVector>
#include <QDebug>

// ============================================================
// MCP Protocol Data Structures (JSON-RPC 2.0 based)
// ============================================================

//
// --- JSON-RPC 2.0 Message Types ---
//

struct MCPCORE_EXPORT MCPJsonRpcMessage
{
    QString jsonrpc = QStringLiteral("2.0");
    virtual ~MCPJsonRpcMessage() = default;
};

struct MCPCORE_EXPORT MCPRequest : public MCPJsonRpcMessage
{
    // JSON-RPC request id (can be int, string, or null)
    QJsonValue id;
    QString method;
    QJsonObject params;

    MCPRequest() = default;
    MCPRequest(const QJsonValue &id, const QString &method, const QJsonObject &params = QJsonObject())
        : id(id), method(method), params(params) {}
};

struct MCPCORE_EXPORT MCPError
{
    int code = 0;
    QString message;
    QJsonValue data; // optional

    MCPError() = default;
    MCPError(int code, const QString &message, const QJsonValue &data = QJsonValue::Undefined)
        : code(code), message(message), data(data) {}
};

struct MCPCORE_EXPORT MCPErrorResponse : public MCPJsonRpcMessage
{
    QJsonValue id;
    MCPError error;
};

struct MCPCORE_EXPORT MCPResponse : public MCPJsonRpcMessage
{
    QJsonValue id;
    QJsonValue result;

    MCPResponse() = default;
    explicit MCPResponse(const QJsonValue &id, const QJsonValue &result = QJsonValue())
        : id(id), result(result) {}
};

struct MCPCORE_EXPORT MCPNotification : public MCPJsonRpcMessage
{
    QString method;
    QJsonObject params;

    MCPNotification() = default;
    MCPNotification(const QString &method, const QJsonObject &params = QJsonObject())
        : method(method), params(params) {}
};

//
// --- MCP Content Types ---
//

struct MCPCORE_EXPORT MCPTextContent
{
    QString type = QStringLiteral("text");
    QString text;

    MCPTextContent() = default;
    explicit MCPTextContent(const QString &text) : text(text) {}

    QJsonObject toJson() const;
    static MCPTextContent fromJson(const QJsonObject &obj);
};

struct MCPCORE_EXPORT MCPImageContent
{
    QString type = QStringLiteral("image");
    QString data;     // base64-encoded data
    QString mimeType; // e.g. "image/png"

    MCPImageContent() = default;
    MCPImageContent(const QString &data, const QString &mimeType)
        : data(data), mimeType(mimeType) {}

    QJsonObject toJson() const;
    static MCPImageContent fromJson(const QJsonObject &obj);
};

struct MCPCORE_EXPORT MCPResourceContent
{
    QString type = QStringLiteral("resource");
    QString uri;
    QString mimeType;
    QString text; // optional text representation

    MCPResourceContent() = default;

    QJsonObject toJson() const;
    static MCPResourceContent fromJson(const QJsonObject &obj);
};

//
// --- MCP Protocol Objects (Tools, Resources, Prompts) ---
//

struct MCPCORE_EXPORT MCPTool
{
    QString name;
    QString description;
    QJsonObject inputSchema; // JSON Schema for tool parameters

    MCPTool() = default;
    MCPTool(const QString &name, const QString &description, const QJsonObject &inputSchema = QJsonObject())
        : name(name), description(description), inputSchema(inputSchema) {}

    QJsonObject toJson() const;
    static MCPTool fromJson(const QJsonObject &obj);
};

struct MCPCORE_EXPORT MCPResource
{
    QString uri;
    QString name;
    QString description;
    QString mimeType;

    MCPResource() = default;
    MCPResource(const QString &uri, const QString &name,
                const QString &description = QString(),
                const QString &mimeType = QString())
        : uri(uri), name(name), description(description), mimeType(mimeType) {}

    QJsonObject toJson() const;
    static MCPResource fromJson(const QJsonObject &obj);
};

struct MCPCORE_EXPORT MCPPromptArgument
{
    QString name;
    QString description;
    bool required = false;

    QJsonObject toJson() const;
    static MCPPromptArgument fromJson(const QJsonObject &obj);
};

struct MCPCORE_EXPORT MCPPrompt
{
    QString name;
    QString description;
    QVector<MCPPromptArgument> arguments;

    MCPPrompt() = default;
    MCPPrompt(const QString &name, const QString &description,
              const QVector<MCPPromptArgument> &arguments = {})
        : name(name), description(description), arguments(arguments) {}

    QJsonObject toJson() const;
    static MCPPrompt fromJson(const QJsonObject &obj);
};

//
// --- Server Capabilities ---
//

struct MCPCORE_EXPORT MCPServerCapabilities
{
    bool tools = false;
    bool resources = false;
    bool prompts = false;
    bool logging = false;

    QJsonObject toJson() const;
    static MCPServerCapabilities fromJson(const QJsonObject &obj);
};

//
// --- Initialize Request / Result ---
//

struct MCPCORE_EXPORT MCPClientInfo
{
    QString name;
    QString version;

    QJsonObject toJson() const;
    static MCPClientInfo fromJson(const QJsonObject &obj);
};

struct MCPCORE_EXPORT MCPServerInfo
{
    QString name;
    QString version;

    QJsonObject toJson() const;
    static MCPServerInfo fromJson(const QJsonObject &obj);
};

struct MCPCORE_EXPORT MCPInitializeRequest
{
    QString protocolVersion = QStringLiteral("2024-11-05");
    MCPClientInfo clientInfo;
    MCPServerCapabilities capabilities;

    QJsonObject toJson() const;
    static MCPInitializeRequest fromJson(const QJsonObject &obj);
};

struct MCPCORE_EXPORT MCPInitializeResult
{
    QString protocolVersion = QStringLiteral("2024-11-05");
    MCPServerInfo serverInfo;
    MCPServerCapabilities capabilities;

    QJsonObject toJson() const;
    static MCPInitializeResult fromJson(const QJsonObject &obj);
};

//
// --- Utility ---
//

namespace MCPUtils
{
    MCPCORE_EXPORT QJsonValue makeId();
    MCPCORE_EXPORT QString messageTypeToString(const QJsonObject &msg);
}

#endif // MCPTYPES_H
