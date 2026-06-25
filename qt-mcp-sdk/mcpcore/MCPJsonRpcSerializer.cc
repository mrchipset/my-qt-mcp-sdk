#include "MCPJsonRpcSerializer.h"

// ============================================================
// Serialization
// ============================================================

QJsonObject MCPJsonRpcSerializer::serializeRequest(const MCPRequest &req)
{
    QJsonObject obj;
    obj[QStringLiteral("jsonrpc")] = QStringLiteral("2.0");
    obj[QStringLiteral("id")] = req.id;
    obj[QStringLiteral("method")] = req.method;
    if (!req.params.isEmpty())
        obj[QStringLiteral("params")] = req.params;
    return obj;
}

QJsonObject MCPJsonRpcSerializer::serializeResponse(const MCPResponse &res)
{
    QJsonObject obj;
    obj[QStringLiteral("jsonrpc")] = QStringLiteral("2.0");
    obj[QStringLiteral("id")] = res.id;
    obj[QStringLiteral("result")] = res.result;
    return obj;
}

QJsonObject MCPJsonRpcSerializer::serializeErrorResponse(const MCPErrorResponse &err)
{
    QJsonObject obj;
    obj[QStringLiteral("jsonrpc")] = QStringLiteral("2.0");
    obj[QStringLiteral("id")] = err.id;
    QJsonObject errObj;
    errObj[QStringLiteral("code")] = err.error.code;
    errObj[QStringLiteral("message")] = err.error.message;
    if (!err.error.data.isUndefined() && !err.error.data.isNull())
        errObj[QStringLiteral("data")] = err.error.data;
    obj[QStringLiteral("error")] = errObj;
    return obj;
}

QJsonObject MCPJsonRpcSerializer::serializeNotification(const MCPNotification &notif)
{
    QJsonObject obj;
    obj[QStringLiteral("jsonrpc")] = QStringLiteral("2.0");
    obj[QStringLiteral("method")] = notif.method;
    if (!notif.params.isEmpty())
        obj[QStringLiteral("params")] = notif.params;
    return obj;
}

// ============================================================
// Identification
// ============================================================

QString MCPJsonRpcSerializer::identifyMessage(const QJsonObject &obj)
{
    if (obj.contains(QStringLiteral("id"))) {
        if (obj.contains(QStringLiteral("error")))
            return QStringLiteral("error_response");
        if (obj.contains(QStringLiteral("result")))
            return QStringLiteral("response");
        if (obj.contains(QStringLiteral("method")))
            return QStringLiteral("request");
        return QStringLiteral("unknown");
    }
    if (obj.contains(QStringLiteral("method")))
        return QStringLiteral("notification");
    return QStringLiteral("unknown");
}

// ============================================================
// Deserialization
// ============================================================

MCPRequest MCPJsonRpcSerializer::parseRequest(const QJsonObject &obj)
{
    MCPRequest req;
    req.id = obj.value(QStringLiteral("id"));
    req.method = obj.value(QStringLiteral("method")).toString();
    req.params = obj.value(QStringLiteral("params")).toObject();
    return req;
}

MCPResponse MCPJsonRpcSerializer::parseResponse(const QJsonObject &obj)
{
    MCPResponse res;
    res.id = obj.value(QStringLiteral("id"));
    res.result = obj.value(QStringLiteral("result"));
    return res;
}

MCPErrorResponse MCPJsonRpcSerializer::parseErrorResponse(const QJsonObject &obj)
{
    MCPErrorResponse err;
    err.id = obj.value(QStringLiteral("id"));
    const QJsonObject errObj = obj.value(QStringLiteral("error")).toObject();
    err.error.code = errObj.value(QStringLiteral("code")).toInt();
    err.error.message = errObj.value(QStringLiteral("message")).toString();
    err.error.data = errObj.value(QStringLiteral("data"));
    return err;
}

MCPNotification MCPJsonRpcSerializer::parseNotification(const QJsonObject &obj)
{
    MCPNotification notif;
    notif.method = obj.value(QStringLiteral("method")).toString();
    notif.params = obj.value(QStringLiteral("params")).toObject();
    return notif;
}
