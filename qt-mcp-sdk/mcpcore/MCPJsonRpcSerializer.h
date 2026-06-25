#ifndef MCPJSONRPCSERIALIZER_H
#define MCPJSONRPCSERIALIZER_H

#include "mcpcore_global.h"
#include "MCPTypes.h"

#include <QJsonObject>
#include <QJsonValue>
#include <optional>

//
// MCPJsonRpcSerializer
//
// Serialize and deserialize JSON-RPC 2.0 messages for the MCP protocol.
//

class MCPCORE_EXPORT MCPJsonRpcSerializer
{
public:
    MCPJsonRpcSerializer() = default;

    // --- Serialization ---

    static QJsonObject serializeRequest(const MCPRequest &req);
    static QJsonObject serializeResponse(const MCPResponse &res);
    static QJsonObject serializeErrorResponse(const MCPErrorResponse &err);
    static QJsonObject serializeNotification(const MCPNotification &notif);

    // --- Deserialization ---

    // Returns the parsed message type string (request/response/error_response/notification)
    // Call the appropriate parse* method based on the type.
    static QString identifyMessage(const QJsonObject &obj);

    static MCPRequest parseRequest(const QJsonObject &obj);
    static MCPResponse parseResponse(const QJsonObject &obj);
    static MCPErrorResponse parseErrorResponse(const QJsonObject &obj);
    static MCPNotification parseNotification(const QJsonObject &obj);
};

#endif // MCPJSONRPCSERIALIZER_H
