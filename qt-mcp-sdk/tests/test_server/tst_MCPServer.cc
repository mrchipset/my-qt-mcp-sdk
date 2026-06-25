#include <QtTest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>

#include "MCPServer.h"
#include "MCPTransport.h"

// ============================================================
// MockTransport — in-memory transport for testing MCPServer
// ============================================================

class MockTransport : public MCPTransport
{
    Q_OBJECT
public:
    explicit MockTransport(QObject *parent = nullptr)
        : MCPTransport(parent) {}

    void open() override { m_open = true; emit opened(); }
    void close() override { m_open = false; emit closed(); }
    bool isOpen() const override { return m_open; }

    void sendMessage(const QJsonObject &message) override {
        m_sent.append(message);
        emit messageSent(message);
    }

    int sentCount() const { return m_sent.size(); }
    QJsonObject sentAt(int index) const { return m_sent.at(index); }
    QList<QJsonObject> allSent() const { return m_sent; }

    // Simulate receiving a message
    void injectMessage(const QJsonObject &msg) { emit messageReceived(msg); }

signals:
    void messageSent(const QJsonObject &message);

private:
    bool m_open = false;
    QList<QJsonObject> m_sent;
};

// ============================================================
// Test Class
// ============================================================

class TestMCPServer : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void testLifecycle();
    void testRegisterTool();
    void testRegisterResource();
    void testRegisterPrompt();
    void testHandleInitialize();
    void testHandleListTools();
    void testHandleCallTool();
    void testHandleCallToolNotFound();
    void testHandleListResources();
    void testHandlePing();
    void testHandleUnknownMethod();
    void testServerInfo();

private:
    MockTransport *m_transport = nullptr;
    MCPServer *m_server = nullptr;
};

void TestMCPServer::init()
{
    auto transport = std::make_unique<MockTransport>();
    m_transport = transport.get();
    m_server = new MCPServer(std::move(transport), this);
}

void TestMCPServer::cleanup()
{
    m_server = nullptr;
    m_transport = nullptr;
}

// ============================================================
// Tests
// ============================================================

void TestMCPServer::testLifecycle()
{
    QSignalSpy startedSpy(m_server, &MCPServer::started);
    QSignalSpy stoppedSpy(m_server, &MCPServer::stopped);

    QVERIFY(!m_server->isRunning());
    m_server->start();
    QVERIFY(m_server->isRunning());
    QCOMPARE(startedSpy.count(), 1);

    m_server->stop();
    QVERIFY(!m_server->isRunning());
    QCOMPARE(stoppedSpy.count(), 1);
}

void TestMCPServer::testRegisterTool()
{
    QJsonObject schema;
    schema[QStringLiteral("type")] = QStringLiteral("object");
    m_server->registerTool(QStringLiteral("echo"), QStringLiteral("Echo input"),
                            schema, [](const QJsonObject &params) { return params; });

    QVERIFY(m_server->hasTool(QStringLiteral("echo")));
    QVERIFY(!m_server->hasTool(QStringLiteral("nonexistent")));
    QCOMPARE(m_server->toolNames().size(), 1);
    QCOMPARE(m_server->toolNames().first(), QStringLiteral("echo"));

    m_server->removeTool(QStringLiteral("echo"));
    QVERIFY(!m_server->hasTool(QStringLiteral("echo")));
}

void TestMCPServer::testRegisterResource()
{
    m_server->registerResource(QStringLiteral("file:///test.txt"), QStringLiteral("Test"),
                                QStringLiteral("Test resource"), QStringLiteral("text/plain"),
                                [](const QString &uri) {
        Q_UNUSED(uri)
        QJsonObject content;
        content[QStringLiteral("uri")] = QStringLiteral("file:///test.txt");
        content[QStringLiteral("text")] = QStringLiteral("Hello");
        return content;
    });

    QVERIFY(m_server->hasResource(QStringLiteral("file:///test.txt")));

    m_server->removeResource(QStringLiteral("file:///test.txt"));
    QVERIFY(!m_server->hasResource(QStringLiteral("file:///test.txt")));
}

void TestMCPServer::testRegisterPrompt()
{
    QJsonObject argsSchema;
    m_server->registerPrompt(QStringLiteral("greet"), QStringLiteral("Greeting"),
                              argsSchema, [](const QJsonObject &args) { return args; });

    QVERIFY(m_server->hasPrompt(QStringLiteral("greet")));

    m_server->removePrompt(QStringLiteral("greet"));
    QVERIFY(!m_server->hasPrompt(QStringLiteral("greet")));
}

void TestMCPServer::testHandleInitialize()
{
    m_server->start();

    // Build initialize request
    QJsonObject initRequest;
    initRequest[QStringLiteral("jsonrpc")] = QStringLiteral("2.0");
    initRequest[QStringLiteral("id")] = 1;
    initRequest[QStringLiteral("method")] = QStringLiteral("initialize");
    QJsonObject params;
    params[QStringLiteral("protocolVersion")] = QStringLiteral("2024-11-05");
    params[QStringLiteral("clientInfo")] = QJsonObject({
        {QStringLiteral("name"), QStringLiteral("TestClient")},
        {QStringLiteral("version"), QStringLiteral("1.0")}
    });
    params[QStringLiteral("capabilities")] = QJsonObject();
    initRequest[QStringLiteral("params")] = params;

    m_transport->injectMessage(initRequest);
    QCOMPARE(m_transport->sentCount(), 1);

    QJsonObject response = m_transport->sentAt(0);
    QCOMPARE(response[QStringLiteral("id")].toInt(), 1);
    QVERIFY(response.contains(QStringLiteral("result")));
    QCOMPARE(response[QStringLiteral("result")].toObject()[QStringLiteral("protocolVersion")].toString(),
             QStringLiteral("2024-11-05"));
}

void TestMCPServer::testHandleListTools()
{
    m_server->start();
    m_server->registerTool(QStringLiteral("tool1"), QStringLiteral("First tool"),
                            QJsonObject(), [](const QJsonObject &p) { return p; });
    m_server->registerTool(QStringLiteral("tool2"), QStringLiteral("Second tool"),
                            QJsonObject(), [](const QJsonObject &p) { return p; });

    QJsonObject listRequest;
    listRequest[QStringLiteral("jsonrpc")] = QStringLiteral("2.0");
    listRequest[QStringLiteral("id")] = 2;
    listRequest[QStringLiteral("method")] = QStringLiteral("tools/list");

    m_transport->injectMessage(listRequest);
    QCOMPARE(m_transport->sentCount(), 1);

    QJsonObject response = m_transport->sentAt(0);
    QJsonArray tools = response[QStringLiteral("result")].toObject()[QStringLiteral("tools")].toArray();
    QCOMPARE(tools.size(), 2);
}

void TestMCPServer::testHandleCallTool()
{
    m_server->start();
    m_server->registerTool(QStringLiteral("echo"), QStringLiteral("Echo"),
                            QJsonObject(), [](const QJsonObject &params) {
        return params;  // echo back
    });

    QJsonObject callRequest;
    callRequest[QStringLiteral("jsonrpc")] = QStringLiteral("2.0");
    callRequest[QStringLiteral("id")] = 3;
    callRequest[QStringLiteral("method")] = QStringLiteral("tools/call");
    QJsonObject params;
    params[QStringLiteral("name")] = QStringLiteral("echo");
    QJsonObject args;
    args[QStringLiteral("message")] = QStringLiteral("Hello, MCP!");
    params[QStringLiteral("arguments")] = args;
    callRequest[QStringLiteral("params")] = params;

    m_transport->injectMessage(callRequest);
    QCOMPARE(m_transport->sentCount(), 1);

    QJsonObject response = m_transport->sentAt(0);
    QJsonObject result = response[QStringLiteral("result")].toObject();
    QJsonArray content = result[QStringLiteral("content")].toArray();
    QCOMPARE(content.size(), 1);
    QVERIFY(content[0].toObject()[QStringLiteral("text")].toString().contains(QStringLiteral("Hello, MCP!")));
}

void TestMCPServer::testHandleCallToolNotFound()
{
    m_server->start();

    QJsonObject callRequest;
    callRequest[QStringLiteral("jsonrpc")] = QStringLiteral("2.0");
    callRequest[QStringLiteral("id")] = 4;
    callRequest[QStringLiteral("method")] = QStringLiteral("tools/call");
    QJsonObject params;
    params[QStringLiteral("name")] = QStringLiteral("nonexistent");
    params[QStringLiteral("arguments")] = QJsonObject();
    callRequest[QStringLiteral("params")] = params;

    m_transport->injectMessage(callRequest);
    QCOMPARE(m_transport->sentCount(), 1);

    QJsonObject response = m_transport->sentAt(0);
    QVERIFY(response.contains(QStringLiteral("error")));
    QCOMPARE(response[QStringLiteral("error")].toObject()[QStringLiteral("code")].toInt(), -32602);
}

void TestMCPServer::testHandleListResources()
{
    m_server->start();

    QJsonObject listRequest;
    listRequest[QStringLiteral("jsonrpc")] = QStringLiteral("2.0");
    listRequest[QStringLiteral("id")] = 5;
    listRequest[QStringLiteral("method")] = QStringLiteral("resources/list");

    m_transport->injectMessage(listRequest);
    QCOMPARE(m_transport->sentCount(), 1);

    QJsonObject response = m_transport->sentAt(0);
    QJsonObject result = response[QStringLiteral("result")].toObject();
    QVERIFY(result.contains(QStringLiteral("resources")));
}

void TestMCPServer::testHandlePing()
{
    m_server->start();

    QJsonObject pingRequest;
    pingRequest[QStringLiteral("jsonrpc")] = QStringLiteral("2.0");
    pingRequest[QStringLiteral("id")] = 6;
    pingRequest[QStringLiteral("method")] = QStringLiteral("ping");

    m_transport->injectMessage(pingRequest);
    QCOMPARE(m_transport->sentCount(), 1);

    QJsonObject response = m_transport->sentAt(0);
    QVERIFY(response.contains(QStringLiteral("result")));
}

void TestMCPServer::testHandleUnknownMethod()
{
    m_server->start();

    QJsonObject badRequest;
    badRequest[QStringLiteral("jsonrpc")] = QStringLiteral("2.0");
    badRequest[QStringLiteral("id")] = 7;
    badRequest[QStringLiteral("method")] = QStringLiteral("unknown/method");

    m_transport->injectMessage(badRequest);
    QCOMPARE(m_transport->sentCount(), 1);

    QJsonObject response = m_transport->sentAt(0);
    QVERIFY(response.contains(QStringLiteral("error")));
    QCOMPARE(response[QStringLiteral("error")].toObject()[QStringLiteral("code")].toInt(), -32601);
}

void TestMCPServer::testServerInfo()
{
    m_server->setServerInfo(QStringLiteral("CustomServer"), QStringLiteral("3.0.0"));
    m_server->setServerCapabilities(true, false, true, false);
    m_server->start();

    QJsonObject initRequest;
    initRequest[QStringLiteral("jsonrpc")] = QStringLiteral("2.0");
    initRequest[QStringLiteral("id")] = 1;
    initRequest[QStringLiteral("method")] = QStringLiteral("initialize");
    initRequest[QStringLiteral("params")] = QJsonObject({
        {QStringLiteral("protocolVersion"), QStringLiteral("2024-11-05")},
        {QStringLiteral("clientInfo"), QJsonObject({
            {QStringLiteral("name"), QStringLiteral("T")},
            {QStringLiteral("version"), QStringLiteral("1.0")}
        })},
        {QStringLiteral("capabilities"), QJsonObject()}
    });

    m_transport->injectMessage(initRequest);
    QJsonObject result = m_transport->sentAt(0)[QStringLiteral("result")].toObject();
    QCOMPARE(result[QStringLiteral("serverInfo")].toObject()[QStringLiteral("name")].toString(), QStringLiteral("CustomServer"));
    QCOMPARE(result[QStringLiteral("serverInfo")].toObject()[QStringLiteral("version")].toString(), QStringLiteral("3.0.0"));
    QVERIFY(result[QStringLiteral("capabilities")].toObject().contains(QStringLiteral("tools")));
    QVERIFY(!result[QStringLiteral("capabilities")].toObject().contains(QStringLiteral("resources")));
    QVERIFY(result[QStringLiteral("capabilities")].toObject().contains(QStringLiteral("prompts")));
}

QTEST_MAIN(TestMCPServer)
#include "tst_MCPServer.moc"
