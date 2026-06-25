#include <QtTest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include "MCPTypes.h"

class TestMCPTypes : public QObject
{
    Q_OBJECT

private slots:
    // --- MCPRequest ---
    void testMCPRequestDefaults();
    void testMCPRequestCustom();
    void testMCPRequestJsonRoundTrip();

    // --- MCPResponse ---
    void testMCPResponseDefaults();
    void testMCPResponseCustom();

    // --- MCPErrorResponse ---
    void testMCPErrorResponse();

    // --- MCPNotification ---
    void testMCPNotificationDefaults();
    void testMCPNotificationCustom();

    // --- MCPTool ---
    void testMCPToolDefaults();
    void testMCPToolCustom();
    void testMCPToolJsonRoundTrip();

    // --- MCPResource ---
    void testMCPResourceDefaults();
    void testMCPResourceJsonRoundTrip();

    // --- MCPPrompt ---
    void testMCPPromptDefaults();
    void testMCPPromptJsonRoundTrip();

    // --- MCPTextContent ---
    void testMCPTextContentJsonRoundTrip();

    // --- MCPServerCapabilities ---
    void testMCPServerCapabilities();

    // --- MCPInitializeRequest / Result ---
    void testMCPInitializeRequest();
    void testMCPInitializeResult();

    // --- MCPUtils ---
    void testMCPUtilsMakeId();
    void testMCPUtilsMessageTypeToString();
};

// ============================================================
// MCPRequest
// ============================================================

void TestMCPTypes::testMCPRequestDefaults()
{
    MCPRequest req;
    QCOMPARE(req.jsonrpc, QStringLiteral("2.0"));
    QVERIFY(req.id.isNull());
    QVERIFY(req.method.isEmpty());
    QVERIFY(req.params.isEmpty());
}

void TestMCPTypes::testMCPRequestCustom()
{
    MCPRequest req(QJsonValue(42), QStringLiteral("test/method"));
    QCOMPARE(req.id.toInt(), 42);
    QCOMPARE(req.method, QStringLiteral("test/method"));
    QVERIFY(req.params.isEmpty());

    QJsonObject params;
    params[QStringLiteral("key")] = QStringLiteral("value");
    MCPRequest req2(QJsonValue(1), QStringLiteral("test/params"), params);
    QCOMPARE(req2.params.value(QStringLiteral("key")).toString(), QStringLiteral("value"));
}

void TestMCPTypes::testMCPRequestJsonRoundTrip()
{
    QJsonObject params;
    params[QStringLiteral("a")] = 1;
    MCPRequest req(QJsonValue(99), QStringLiteral("test/roundtrip"), params);

    QJsonObject obj;
    obj[QStringLiteral("jsonrpc")] = QStringLiteral("2.0");
    obj[QStringLiteral("id")] = req.id;
    obj[QStringLiteral("method")] = req.method;
    obj[QStringLiteral("params")] = req.params;

    QCOMPARE(obj[QStringLiteral("jsonrpc")].toString(), QStringLiteral("2.0"));
    QCOMPARE(obj[QStringLiteral("id")].toInt(), 99);
    QCOMPARE(obj[QStringLiteral("method")].toString(), QStringLiteral("test/roundtrip"));
    QCOMPARE(obj[QStringLiteral("params")].toObject()[QStringLiteral("a")].toInt(), 1);
}

// ============================================================
// MCPResponse
// ============================================================

void TestMCPTypes::testMCPResponseDefaults()
{
    MCPResponse res;
    QCOMPARE(res.jsonrpc, QStringLiteral("2.0"));
    QVERIFY(res.id.isNull());
    QVERIFY(res.result.isNull());
}

void TestMCPTypes::testMCPResponseCustom()
{
    QJsonObject result;
    result[QStringLiteral("ok")] = true;
    MCPResponse res(QJsonValue(1), result);
    QCOMPARE(res.id.toInt(), 1);
    QVERIFY(res.result.toObject()[QStringLiteral("ok")].toBool());
}

// ============================================================
// MCPErrorResponse
// ============================================================

void TestMCPTypes::testMCPErrorResponse()
{
    MCPErrorResponse err;
    err.id = QJsonValue(5);
    err.error = MCPError(-32600, QStringLiteral("Invalid Request"));
    QCOMPARE(err.id.toInt(), 5);
    QCOMPARE(err.error.code, -32600);
    QCOMPARE(err.error.message, QStringLiteral("Invalid Request"));
    QVERIFY(err.error.data.isUndefined());

    err.error.data = QJsonObject({{QStringLiteral("detail"), QStringLiteral("missing field")}});
    QVERIFY(!err.error.data.isUndefined());
}

// ============================================================
// MCPNotification
// ============================================================

void TestMCPTypes::testMCPNotificationDefaults()
{
    MCPNotification notif;
    QCOMPARE(notif.jsonrpc, QStringLiteral("2.0"));
    QVERIFY(notif.method.isEmpty());
    QVERIFY(notif.params.isEmpty());
}

void TestMCPTypes::testMCPNotificationCustom()
{
    QJsonObject params;
    params[QStringLiteral("level")] = QStringLiteral("info");
    MCPNotification notif(QStringLiteral("notifications/log"), params);
    QCOMPARE(notif.method, QStringLiteral("notifications/log"));
    QCOMPARE(notif.params[QStringLiteral("level")].toString(), QStringLiteral("info"));
}

// ============================================================
// MCPTool
// ============================================================

void TestMCPTypes::testMCPToolDefaults()
{
    MCPTool tool;
    QVERIFY(tool.name.isEmpty());
    QVERIFY(tool.description.isEmpty());
    QVERIFY(tool.inputSchema.isEmpty());
}

void TestMCPTypes::testMCPToolCustom()
{
    QJsonObject schema;
    schema[QStringLiteral("type")] = QStringLiteral("object");
    MCPTool tool(QStringLiteral("echo"), QStringLiteral("Echo input back"), schema);
    QCOMPARE(tool.name, QStringLiteral("echo"));
    QCOMPARE(tool.description, QStringLiteral("Echo input back"));
    QCOMPARE(tool.inputSchema[QStringLiteral("type")].toString(), QStringLiteral("object"));
}

void TestMCPTypes::testMCPToolJsonRoundTrip()
{
    QJsonObject schema;
    schema[QStringLiteral("type")] = QStringLiteral("object");
    MCPTool tool(QStringLiteral("calc"), QStringLiteral("Calculator"), schema);
    QJsonObject json = tool.toJson();
    QCOMPARE(json[QStringLiteral("name")].toString(), QStringLiteral("calc"));
    QCOMPARE(json[QStringLiteral("description")].toString(), QStringLiteral("Calculator"));

    MCPTool parsed = MCPTool::fromJson(json);
    QCOMPARE(parsed.name, tool.name);
    QCOMPARE(parsed.description, tool.description);
    QCOMPARE(parsed.inputSchema[QStringLiteral("type")].toString(), QStringLiteral("object"));
}

// ============================================================
// MCPResource
// ============================================================

void TestMCPTypes::testMCPResourceDefaults()
{
    MCPResource res;
    QVERIFY(res.uri.isEmpty());
    QVERIFY(res.name.isEmpty());
}

void TestMCPTypes::testMCPResourceJsonRoundTrip()
{
    MCPResource res(QStringLiteral("file:///data.txt"), QStringLiteral("Data"),
                    QStringLiteral("Sample data"), QStringLiteral("text/plain"));
    QJsonObject json = res.toJson();
    QCOMPARE(json[QStringLiteral("uri")].toString(), QStringLiteral("file:///data.txt"));
    QCOMPARE(json[QStringLiteral("name")].toString(), QStringLiteral("Data"));
    QCOMPARE(json[QStringLiteral("description")].toString(), QStringLiteral("Sample data"));
    QCOMPARE(json[QStringLiteral("mimeType")].toString(), QStringLiteral("text/plain"));

    MCPResource parsed = MCPResource::fromJson(json);
    QCOMPARE(parsed.uri, res.uri);
    QCOMPARE(parsed.name, res.name);
    QCOMPARE(parsed.description, res.description);
    QCOMPARE(parsed.mimeType, res.mimeType);
}

// ============================================================
// MCPPrompt
// ============================================================

void TestMCPTypes::testMCPPromptDefaults()
{
    MCPPrompt prompt;
    QVERIFY(prompt.name.isEmpty());
    QVERIFY(prompt.arguments.isEmpty());
}

void TestMCPTypes::testMCPPromptJsonRoundTrip()
{
    QVector<MCPPromptArgument> args;
    MCPPromptArgument arg;
    arg.name = QStringLiteral("input");
    arg.description = QStringLiteral("Input text");
    arg.required = true;
    args.append(arg);

    MCPPrompt prompt(QStringLiteral("greet"), QStringLiteral("Greet the user"), args);
    QJsonObject json = prompt.toJson();

    QCOMPARE(json[QStringLiteral("name")].toString(), QStringLiteral("greet"));
    QJsonArray arr = json[QStringLiteral("arguments")].toArray();
    QCOMPARE(arr.size(), 1);
    QCOMPARE(arr[0].toObject()[QStringLiteral("name")].toString(), QStringLiteral("input"));

    MCPPrompt parsed = MCPPrompt::fromJson(json);
    QCOMPARE(parsed.name, prompt.name);
    QCOMPARE(parsed.arguments.size(), 1);
    QCOMPARE(parsed.arguments[0].name, QStringLiteral("input"));
    QVERIFY(parsed.arguments[0].required);
}

// ============================================================
// MCPTextContent
// ============================================================

void TestMCPTypes::testMCPTextContentJsonRoundTrip()
{
    MCPTextContent content(QStringLiteral("Hello, MCP!"));
    QJsonObject json = content.toJson();
    QCOMPARE(json[QStringLiteral("type")].toString(), QStringLiteral("text"));
    QCOMPARE(json[QStringLiteral("text")].toString(), QStringLiteral("Hello, MCP!"));

    MCPTextContent parsed = MCPTextContent::fromJson(json);
    QCOMPARE(parsed.text, QStringLiteral("Hello, MCP!"));
}

// ============================================================
// MCPServerCapabilities
// ============================================================

void TestMCPTypes::testMCPServerCapabilities()
{
    MCPServerCapabilities caps;
    caps.tools = true;
    caps.resources = true;
    caps.prompts = false;
    caps.logging = false;

    QJsonObject json = caps.toJson();
    QVERIFY(json.contains(QStringLiteral("tools")));
    QVERIFY(json.contains(QStringLiteral("resources")));
    QVERIFY(!json.contains(QStringLiteral("prompts")));
    QVERIFY(!json.contains(QStringLiteral("logging")));

    MCPServerCapabilities parsed = MCPServerCapabilities::fromJson(json);
    QVERIFY(parsed.tools);
    QVERIFY(parsed.resources);
    QVERIFY(!parsed.prompts);
    QVERIFY(!parsed.logging);
}

// ============================================================
// MCPInitializeRequest / Result
// ============================================================

void TestMCPTypes::testMCPInitializeRequest()
{
    MCPInitializeRequest req;
    req.clientInfo.name = QStringLiteral("TestClient");
    req.clientInfo.version = QStringLiteral("1.0");
    req.capabilities.tools = true;

    QJsonObject json = req.toJson();
    QCOMPARE(json[QStringLiteral("protocolVersion")].toString(), QStringLiteral("2024-11-05"));
    QCOMPARE(json[QStringLiteral("clientInfo")].toObject()[QStringLiteral("name")].toString(),
             QStringLiteral("TestClient"));

    MCPInitializeRequest parsed = MCPInitializeRequest::fromJson(json);
    QCOMPARE(parsed.clientInfo.name, QStringLiteral("TestClient"));
    QVERIFY(parsed.capabilities.tools);
}

void TestMCPTypes::testMCPInitializeResult()
{
    MCPInitializeResult res;
    res.serverInfo.name = QStringLiteral("TestServer");
    res.serverInfo.version = QStringLiteral("2.0");

    QJsonObject json = res.toJson();
    QCOMPARE(json[QStringLiteral("serverInfo")].toObject()[QStringLiteral("name")].toString(),
             QStringLiteral("TestServer"));

    MCPInitializeResult parsed = MCPInitializeResult::fromJson(json);
    QCOMPARE(parsed.serverInfo.name, QStringLiteral("TestServer"));
    QCOMPARE(parsed.serverInfo.version, QStringLiteral("2.0"));
}

// ============================================================
// MCPUtils
// ============================================================

void TestMCPTypes::testMCPUtilsMakeId()
{
    QJsonValue id1 = MCPUtils::makeId();
    QJsonValue id2 = MCPUtils::makeId();
    QVERIFY(id1.isDouble());
    QVERIFY(id2.isDouble());
    // Two consecutive calls should produce different ids
    QVERIFY(id1.toDouble() != id2.toDouble());
}

void TestMCPTypes::testMCPUtilsMessageTypeToString()
{
    QJsonObject request;
    request[QStringLiteral("jsonrpc")] = QStringLiteral("2.0");
    request[QStringLiteral("id")] = 1;
    request[QStringLiteral("method")] = QStringLiteral("ping");
    QCOMPARE(MCPUtils::messageTypeToString(request), QStringLiteral("request"));

    QJsonObject response;
    response[QStringLiteral("jsonrpc")] = QStringLiteral("2.0");
    response[QStringLiteral("id")] = 1;
    response[QStringLiteral("result")] = QJsonObject();
    QCOMPARE(MCPUtils::messageTypeToString(response), QStringLiteral("response"));

    QJsonObject error;
    error[QStringLiteral("jsonrpc")] = QStringLiteral("2.0");
    error[QStringLiteral("id")] = 1;
    error[QStringLiteral("error")] = QJsonObject({{QStringLiteral("code"), -32600}});
    QCOMPARE(MCPUtils::messageTypeToString(error), QStringLiteral("error_response"));

    QJsonObject notification;
    notification[QStringLiteral("jsonrpc")] = QStringLiteral("2.0");
    notification[QStringLiteral("method")] = QStringLiteral("notifications/log");
    QCOMPARE(MCPUtils::messageTypeToString(notification), QStringLiteral("notification"));
}

QTEST_MAIN(TestMCPTypes)
#include "tst_MCPTypes.moc"
