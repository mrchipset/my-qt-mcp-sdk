#include "MCPTypes.h"
#include <QRandomGenerator>
#include <QJsonDocument>

// ============================================================
// MCPTextContent
// ============================================================

QJsonObject MCPTextContent::toJson() const
{
    QJsonObject obj;
    obj[QStringLiteral("type")] = type;
    obj[QStringLiteral("text")] = text;
    return obj;
}

MCPTextContent MCPTextContent::fromJson(const QJsonObject &obj)
{
    MCPTextContent content;
    content.text = obj.value(QStringLiteral("text")).toString();
    return content;
}

// ============================================================
// MCPImageContent
// ============================================================

QJsonObject MCPImageContent::toJson() const
{
    QJsonObject obj;
    obj[QStringLiteral("type")] = type;
    obj[QStringLiteral("data")] = data;
    obj[QStringLiteral("mimeType")] = mimeType;
    return obj;
}

MCPImageContent MCPImageContent::fromJson(const QJsonObject &obj)
{
    MCPImageContent content;
    content.data = obj.value(QStringLiteral("data")).toString();
    content.mimeType = obj.value(QStringLiteral("mimeType")).toString();
    return content;
}

// ============================================================
// MCPResourceContent
// ============================================================

QJsonObject MCPResourceContent::toJson() const
{
    QJsonObject obj;
    obj[QStringLiteral("type")] = type;
    obj[QStringLiteral("uri")] = uri;
    obj[QStringLiteral("mimeType")] = mimeType;
    if (!text.isEmpty())
        obj[QStringLiteral("text")] = text;
    return obj;
}

MCPResourceContent MCPResourceContent::fromJson(const QJsonObject &obj)
{
    MCPResourceContent content;
    content.uri = obj.value(QStringLiteral("uri")).toString();
    content.mimeType = obj.value(QStringLiteral("mimeType")).toString();
    content.text = obj.value(QStringLiteral("text")).toString();
    return content;
}

// ============================================================
// MCPTool
// ============================================================

QJsonObject MCPTool::toJson() const
{
    QJsonObject obj;
    obj[QStringLiteral("name")] = name;
    obj[QStringLiteral("description")] = description;
    if (!inputSchema.isEmpty())
        obj[QStringLiteral("inputSchema")] = inputSchema;
    return obj;
}

MCPTool MCPTool::fromJson(const QJsonObject &obj)
{
    MCPTool tool;
    tool.name = obj.value(QStringLiteral("name")).toString();
    tool.description = obj.value(QStringLiteral("description")).toString();
    tool.inputSchema = obj.value(QStringLiteral("inputSchema")).toObject();
    return tool;
}

// ============================================================
// MCPResource
// ============================================================

QJsonObject MCPResource::toJson() const
{
    QJsonObject obj;
    obj[QStringLiteral("uri")] = uri;
    obj[QStringLiteral("name")] = name;
    if (!description.isEmpty())
        obj[QStringLiteral("description")] = description;
    if (!mimeType.isEmpty())
        obj[QStringLiteral("mimeType")] = mimeType;
    return obj;
}

MCPResource MCPResource::fromJson(const QJsonObject &obj)
{
    MCPResource resource;
    resource.uri = obj.value(QStringLiteral("uri")).toString();
    resource.name = obj.value(QStringLiteral("name")).toString();
    resource.description = obj.value(QStringLiteral("description")).toString();
    resource.mimeType = obj.value(QStringLiteral("mimeType")).toString();
    return resource;
}

// ============================================================
// MCPPromptArgument
// ============================================================

QJsonObject MCPPromptArgument::toJson() const
{
    QJsonObject obj;
    obj[QStringLiteral("name")] = name;
    if (!description.isEmpty())
        obj[QStringLiteral("description")] = description;
    if (required)
        obj[QStringLiteral("required")] = true;
    return obj;
}

MCPPromptArgument MCPPromptArgument::fromJson(const QJsonObject &obj)
{
    MCPPromptArgument arg;
    arg.name = obj.value(QStringLiteral("name")).toString();
    arg.description = obj.value(QStringLiteral("description")).toString();
    arg.required = obj.value(QStringLiteral("required")).toBool(false);
    return arg;
}

// ============================================================
// MCPPrompt
// ============================================================

QJsonObject MCPPrompt::toJson() const
{
    QJsonObject obj;
    obj[QStringLiteral("name")] = name;
    if (!description.isEmpty())
        obj[QStringLiteral("description")] = description;
    if (!arguments.isEmpty()) {
        QJsonArray arr;
        for (const auto &arg : arguments)
            arr.append(arg.toJson());
        obj[QStringLiteral("arguments")] = arr;
    }
    return obj;
}

MCPPrompt MCPPrompt::fromJson(const QJsonObject &obj)
{
    MCPPrompt prompt;
    prompt.name = obj.value(QStringLiteral("name")).toString();
    prompt.description = obj.value(QStringLiteral("description")).toString();
    const QJsonArray arr = obj.value(QStringLiteral("arguments")).toArray();
    for (const auto &val : arr)
        prompt.arguments.append(MCPPromptArgument::fromJson(val.toObject()));
    return prompt;
}

// ============================================================
// MCPServerCapabilities
// ============================================================

QJsonObject MCPServerCapabilities::toJson() const
{
    QJsonObject obj;
    if (tools)
        obj[QStringLiteral("tools")] = QJsonObject();
    if (resources)
        obj[QStringLiteral("resources")] = QJsonObject();
    if (prompts)
        obj[QStringLiteral("prompts")] = QJsonObject();
    if (logging)
        obj[QStringLiteral("logging")] = QJsonObject();
    return obj;
}

MCPServerCapabilities MCPServerCapabilities::fromJson(const QJsonObject &obj)
{
    MCPServerCapabilities caps;
    caps.tools = obj.contains(QStringLiteral("tools"));
    caps.resources = obj.contains(QStringLiteral("resources"));
    caps.prompts = obj.contains(QStringLiteral("prompts"));
    caps.logging = obj.contains(QStringLiteral("logging"));
    return caps;
}

// ============================================================
// MCPClientInfo
// ============================================================

QJsonObject MCPClientInfo::toJson() const
{
    QJsonObject obj;
    obj[QStringLiteral("name")] = name;
    obj[QStringLiteral("version")] = version;
    return obj;
}

MCPClientInfo MCPClientInfo::fromJson(const QJsonObject &obj)
{
    MCPClientInfo info;
    info.name = obj.value(QStringLiteral("name")).toString();
    info.version = obj.value(QStringLiteral("version")).toString();
    return info;
}

// ============================================================
// MCPServerInfo
// ============================================================

QJsonObject MCPServerInfo::toJson() const
{
    QJsonObject obj;
    obj[QStringLiteral("name")] = name;
    obj[QStringLiteral("version")] = version;
    return obj;
}

MCPServerInfo MCPServerInfo::fromJson(const QJsonObject &obj)
{
    MCPServerInfo info;
    info.name = obj.value(QStringLiteral("name")).toString();
    info.version = obj.value(QStringLiteral("version")).toString();
    return info;
}

// ============================================================
// MCPInitializeRequest
// ============================================================

QJsonObject MCPInitializeRequest::toJson() const
{
    QJsonObject obj;
    obj[QStringLiteral("protocolVersion")] = protocolVersion;
    obj[QStringLiteral("clientInfo")] = clientInfo.toJson();
    obj[QStringLiteral("capabilities")] = capabilities.toJson();
    return obj;
}

MCPInitializeRequest MCPInitializeRequest::fromJson(const QJsonObject &obj)
{
    MCPInitializeRequest req;
    req.protocolVersion = obj.value(QStringLiteral("protocolVersion")).toString(QStringLiteral("2024-11-05"));
    req.clientInfo = MCPClientInfo::fromJson(obj.value(QStringLiteral("clientInfo")).toObject());
    req.capabilities = MCPServerCapabilities::fromJson(obj.value(QStringLiteral("capabilities")).toObject());
    return req;
}

// ============================================================
// MCPInitializeResult
// ============================================================

QJsonObject MCPInitializeResult::toJson() const
{
    QJsonObject obj;
    obj[QStringLiteral("protocolVersion")] = protocolVersion;
    obj[QStringLiteral("serverInfo")] = serverInfo.toJson();
    obj[QStringLiteral("capabilities")] = capabilities.toJson();
    return obj;
}

MCPInitializeResult MCPInitializeResult::fromJson(const QJsonObject &obj)
{
    MCPInitializeResult res;
    res.protocolVersion = obj.value(QStringLiteral("protocolVersion")).toString(QStringLiteral("2024-11-05"));
    res.serverInfo = MCPServerInfo::fromJson(obj.value(QStringLiteral("serverInfo")).toObject());
    res.capabilities = MCPServerCapabilities::fromJson(obj.value(QStringLiteral("capabilities")).toObject());
    return res;
}

// ============================================================
// Utility
// ============================================================

QJsonValue MCPUtils::makeId()
{
    // Generate a unique integer id
    return QJsonValue(static_cast<qint64>(QRandomGenerator::global()->bounded(1, 2147483647)));
}

QString MCPUtils::messageTypeToString(const QJsonObject &msg)
{
    if (msg.contains(QStringLiteral("id"))) {
        if (msg.contains(QStringLiteral("error")))
            return QStringLiteral("error_response");
        if (msg.contains(QStringLiteral("result")))
            return QStringLiteral("response");
        if (msg.contains(QStringLiteral("method")))
            return QStringLiteral("request");
        return QStringLiteral("unknown");
    }
    if (msg.contains(QStringLiteral("method")))
        return QStringLiteral("notification");
    return QStringLiteral("unknown");
}
