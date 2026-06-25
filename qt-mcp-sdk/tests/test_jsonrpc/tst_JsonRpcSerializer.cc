#include <QtTest>
#include <QJsonDocument>
#include <QJsonObject>

#include "MCPJsonRpcSerializer.h"

class TestJsonRpcSerializer : public QObject
{
    Q_OBJECT

private slots:
    void testSerializeRequest();
    void testSerializeResponse();
    void testSerializeErrorResponse();
    void testSerializeNotification();
    void testIdentifyMessage();
    void testParseRequest();
    void testParseResponse();
    void testParseErrorResponse();
    void testParseNotification();
    void testRoundTripRequest();
    void testRoundTripResponse();
    void testRoundTripNotification();
};

// ============================================================
// Serialization Tests
// ============================================================

void TestJsonRpcSerializer::testSerializeRequest()
{
    QJsonObject params;
    params[QStringLiteral("key")] = QStringLiteral("value");
    MCPRequest req(QJsonValue(1), QStringLiteral("test/method"), params);

    QJsonObject obj = MCPJsonRpcSerializer::serializeRequest(req);
    QCOMPARE(obj[QStringLiteral("jsonrpc")].toString(), QStringLiteral("2.0"));
    QCOMPARE(obj[QStringLiteral("id")].toInt(), 1);
    QCOMPARE(obj[QStringLiteral("method")].toString(), QStringLiteral("test/method"));
    QCOMPARE(obj[QStringLiteral("params")].toObject()[QStringLiteral("key")].toString(), QStringLiteral("value"));
}

void TestJsonRpcSerializer::testSerializeResponse()
{
    QJsonObject result;
    result[QStringLiteral("ok")] = true;
    MCPResponse res(QJsonValue(42), result);

    QJsonObject obj = MCPJsonRpcSerializer::serializeResponse(res);
    QCOMPARE(obj[QStringLiteral("jsonrpc")].toString(), QStringLiteral("2.0"));
    QCOMPARE(obj[QStringLiteral("id")].toInt(), 42);
    QVERIFY(obj[QStringLiteral("result")].toObject()[QStringLiteral("ok")].toBool());
}

void TestJsonRpcSerializer::testSerializeErrorResponse()
{
    MCPErrorResponse err;
    err.id = QJsonValue(1);
    err.error = MCPError(-32600, QStringLiteral("Invalid Request"),
                          QJsonObject({{QStringLiteral("detail"), QStringLiteral("missing field")}}));

    QJsonObject obj = MCPJsonRpcSerializer::serializeErrorResponse(err);
    QCOMPARE(obj[QStringLiteral("jsonrpc")].toString(), QStringLiteral("2.0"));
    QCOMPARE(obj[QStringLiteral("id")].toInt(), 1);
    QCOMPARE(obj[QStringLiteral("error")].toObject()[QStringLiteral("code")].toInt(), -32600);
    QCOMPARE(obj[QStringLiteral("error")].toObject()[QStringLiteral("message")].toString(), QStringLiteral("Invalid Request"));
}

void TestJsonRpcSerializer::testSerializeNotification()
{
    QJsonObject params;
    params[QStringLiteral("level")] = QStringLiteral("info");
    MCPNotification notif(QStringLiteral("notifications/log"), params);

    QJsonObject obj = MCPJsonRpcSerializer::serializeNotification(notif);
    QCOMPARE(obj[QStringLiteral("jsonrpc")].toString(), QStringLiteral("2.0"));
    QCOMPARE(obj[QStringLiteral("method")].toString(), QStringLiteral("notifications/log"));
    QCOMPARE(obj[QStringLiteral("params")].toObject()[QStringLiteral("level")].toString(), QStringLiteral("info"));
    // Notifications should NOT have an "id" field
    QVERIFY(!obj.contains(QStringLiteral("id")));
}

// ============================================================
// Identification Tests
// ============================================================

void TestJsonRpcSerializer::testIdentifyMessage()
{
    QJsonObject req;
    req[QStringLiteral("id")] = 1;
    req[QStringLiteral("method")] = QStringLiteral("ping");
    QCOMPARE(MCPJsonRpcSerializer::identifyMessage(req), QStringLiteral("request"));

    QJsonObject res;
    res[QStringLiteral("id")] = 1;
    res[QStringLiteral("result")] = QJsonObject();
    QCOMPARE(MCPJsonRpcSerializer::identifyMessage(res), QStringLiteral("response"));

    QJsonObject err;
    err[QStringLiteral("id")] = 1;
    err[QStringLiteral("error")] = QJsonObject();
    QCOMPARE(MCPJsonRpcSerializer::identifyMessage(err), QStringLiteral("error_response"));

    QJsonObject notif;
    notif[QStringLiteral("method")] = QStringLiteral("notifications/log");
    QCOMPARE(MCPJsonRpcSerializer::identifyMessage(notif), QStringLiteral("notification"));

    QJsonObject unknown;
    QCOMPARE(MCPJsonRpcSerializer::identifyMessage(unknown), QStringLiteral("unknown"));
}

// ============================================================
// Deserialization Tests
// ============================================================

void TestJsonRpcSerializer::testParseRequest()
{
    QJsonObject obj;
    obj[QStringLiteral("jsonrpc")] = QStringLiteral("2.0");
    obj[QStringLiteral("id")] = 5;
    obj[QStringLiteral("method")] = QStringLiteral("test/parse");

    MCPRequest req = MCPJsonRpcSerializer::parseRequest(obj);
    QCOMPARE(req.id.toInt(), 5);
    QCOMPARE(req.method, QStringLiteral("test/parse"));
}

void TestJsonRpcSerializer::testParseResponse()
{
    QJsonObject obj;
    obj[QStringLiteral("jsonrpc")] = QStringLiteral("2.0");
    obj[QStringLiteral("id")] = 10;
    obj[QStringLiteral("result")] = QJsonObject({{QStringLiteral("data"), 42}});

    MCPResponse res = MCPJsonRpcSerializer::parseResponse(obj);
    QCOMPARE(res.id.toInt(), 10);
    QCOMPARE(res.result.toObject()[QStringLiteral("data")].toInt(), 42);
}

void TestJsonRpcSerializer::testParseErrorResponse()
{
    QJsonObject errObj;
    errObj[QStringLiteral("code")] = -32601;
    errObj[QStringLiteral("message")] = QStringLiteral("Method not found");

    QJsonObject obj;
    obj[QStringLiteral("jsonrpc")] = QStringLiteral("2.0");
    obj[QStringLiteral("id")] = QJsonValue::Null;
    obj[QStringLiteral("error")] = errObj;

    MCPErrorResponse err = MCPJsonRpcSerializer::parseErrorResponse(obj);
    QVERIFY(err.id.isNull());
    QCOMPARE(err.error.code, -32601);
    QCOMPARE(err.error.message, QStringLiteral("Method not found"));
}

void TestJsonRpcSerializer::testParseNotification()
{
    QJsonObject obj;
    obj[QStringLiteral("jsonrpc")] = QStringLiteral("2.0");
    obj[QStringLiteral("method")] = QStringLiteral("notifications/test");

    MCPNotification notif = MCPJsonRpcSerializer::parseNotification(obj);
    QCOMPARE(notif.method, QStringLiteral("notifications/test"));
    QVERIFY(notif.params.isEmpty());
}

// ============================================================
// Round-trip Tests (Serialize → JSON → Parse)
// ============================================================

void TestJsonRpcSerializer::testRoundTripRequest()
{
    QJsonObject params;
    params[QStringLiteral("x")] = 100;
    MCPRequest req(QJsonValue(1), QStringLiteral("test/roundtrip"), params);

    QJsonObject json = MCPJsonRpcSerializer::serializeRequest(req);
    MCPRequest parsed = MCPJsonRpcSerializer::parseRequest(json);

    QCOMPARE(parsed.id.toInt(), req.id.toInt());
    QCOMPARE(parsed.method, req.method);
    QCOMPARE(parsed.params[QStringLiteral("x")].toInt(), 100);
}

void TestJsonRpcSerializer::testRoundTripResponse()
{
    QJsonObject result;
    result[QStringLiteral("message")] = QStringLiteral("ok");
    MCPResponse res(QJsonValue(2), result);

    QJsonObject json = MCPJsonRpcSerializer::serializeResponse(res);
    MCPResponse parsed = MCPJsonRpcSerializer::parseResponse(json);

    QCOMPARE(parsed.id.toInt(), res.id.toInt());
    QCOMPARE(parsed.result.toObject()[QStringLiteral("message")].toString(), QStringLiteral("ok"));
}

void TestJsonRpcSerializer::testRoundTripNotification()
{
    QJsonObject params;
    params[QStringLiteral("event")] = QStringLiteral("update");
    MCPNotification notif(QStringLiteral("notifications/event"), params);

    QJsonObject json = MCPJsonRpcSerializer::serializeNotification(notif);
    MCPNotification parsed = MCPJsonRpcSerializer::parseNotification(json);

    QCOMPARE(parsed.method, notif.method);
    QCOMPARE(parsed.params[QStringLiteral("event")].toString(), QStringLiteral("update"));
}

QTEST_MAIN(TestJsonRpcSerializer)
#include "tst_JsonRpcSerializer.moc"
