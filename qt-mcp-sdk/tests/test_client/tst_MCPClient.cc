#include <QtTest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>
#include <QEventLoop>
#include <QTimer>

#include "MCPClient.h"
#include "MCPTransport.h"

// ============================================================
// MockTransport — in-memory transport for testing MCPClient
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

class TestMCPClient : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void testLifecycle();
    void testSendRequest();
    void testSendRequestNoCallback();
    void testSendNotification();
    void testHandleResponse();
    void testHandleError();
    void testHandleNotification();
    void testInitialize();
    void testPing();
    void testListTools();
    void testCallTool();
    void testListResources();
    void testMultipleRequests();

private:
    MockTransport *m_transport = nullptr;
    MCPClient *m_client = nullptr;
};

void TestMCPClient::init()
{
    auto transport = std::make_unique<MockTransport>();
    m_transport = transport.get();
    m_client = new MCPClient(std::move(transport), this);
}

void TestMCPClient::cleanup()
{
    m_client = nullptr;
    m_transport = nullptr;
}

// ============================================================
// Tests
// ============================================================

void TestMCPClient::testLifecycle()
{
    QSignalSpy connectedSpy(m_client, &MCPClient::connected);
    QSignalSpy disconnectedSpy(m_client, &MCPClient::disconnected);

    QVERIFY(!m_client->isConnected());
    m_client->openConnection();
    QVERIFY(m_client->isConnected());
    QCOMPARE(connectedSpy.count(), 1);

    m_client->closeConnection();
    QVERIFY(!m_client->isConnected());
    QCOMPARE(disconnectedSpy.count(), 1);
}

void TestMCPClient::testSendRequest()
{
    m_client->openConnection();

    bool callbackCalled = false;
    m_client->sendRequest(QStringLiteral("test/method"), QJsonObject(),
                           [&](bool success, const QJsonValue &result, int code, const QString &msg) {
        Q_UNUSED(success)
        Q_UNUSED(result)
        Q_UNUSED(code)
        Q_UNUSED(msg)
        callbackCalled = true;
    });

    QCOMPARE(m_transport->sentCount(), 1);

    // Send a response back to trigger callback
    QJsonObject response;
    response[QStringLiteral("jsonrpc")] = QStringLiteral("2.0");
    response[QStringLiteral("id")] = m_transport->sentAt(0)[QStringLiteral("id")];
    response[QStringLiteral("result")] = QJsonObject({{QStringLiteral("ok"), true}});
    m_transport->injectMessage(response);

    QVERIFY(callbackCalled);
}

void TestMCPClient::testSendRequestNoCallback()
{
    m_client->openConnection();
    m_client->sendRequest(QStringLiteral("test/nocb"), QJsonObject(), nullptr);
    QCOMPARE(m_transport->sentCount(), 1);
    QCOMPARE(m_transport->sentAt(0)[QStringLiteral("method")].toString(), QStringLiteral("test/nocb"));
}

void TestMCPClient::testSendNotification()
{
    m_client->openConnection();

    QJsonObject params;
    params[QStringLiteral("level")] = QStringLiteral("info");
    m_client->sendNotification(QStringLiteral("notifications/log"), params);
    QCOMPARE(m_transport->sentCount(), 1);

    QJsonObject msg = m_transport->sentAt(0);
    QCOMPARE(msg[QStringLiteral("method")].toString(), QStringLiteral("notifications/log"));
    QVERIFY(!msg.contains(QStringLiteral("id")));  // notifications don't have id
}

void TestMCPClient::testHandleResponse()
{
    m_client->openConnection();

    bool success = false;
    QJsonValue resultValue;
    m_client->sendRequest(QStringLiteral("test/rsp"), QJsonObject(),
                           [&](bool s, const QJsonValue &r, int, const QString &) {
        success = s;
        resultValue = r;
    });

    QJsonObject response;
    response[QStringLiteral("jsonrpc")] = QStringLiteral("2.0");
    response[QStringLiteral("id")] = m_transport->sentAt(0)[QStringLiteral("id")];
    response[QStringLiteral("result")] = QJsonObject({{QStringLiteral("data"), 42}});
    m_transport->injectMessage(response);

    QVERIFY(success);
    QCOMPARE(resultValue.toObject()[QStringLiteral("data")].toInt(), 42);
}

void TestMCPClient::testHandleError()
{
    m_client->openConnection();

    bool success = true;
    int errorCode = 0;
    QString errorMsg;
    m_client->sendRequest(QStringLiteral("test/err"), QJsonObject(),
                           [&](bool s, const QJsonValue &, int c, const QString &m) {
        success = s;
        errorCode = c;
        errorMsg = m;
    });

    QJsonObject errorResponse;
    errorResponse[QStringLiteral("jsonrpc")] = QStringLiteral("2.0");
    errorResponse[QStringLiteral("id")] = m_transport->sentAt(0)[QStringLiteral("id")];
    QJsonObject error;
    error[QStringLiteral("code")] = -32601;
    error[QStringLiteral("message")] = QStringLiteral("Method not found");
    errorResponse[QStringLiteral("error")] = error;
    m_transport->injectMessage(errorResponse);

    QVERIFY(!success);
    QCOMPARE(errorCode, -32601);
    QCOMPARE(errorMsg, QStringLiteral("Method not found"));
}

void TestMCPClient::testHandleNotification()
{
    m_client->openConnection();

    QString receivedMethod;
    QJsonObject receivedParams;
    m_client->setNotificationHandler([&](const QString &method, const QJsonObject &params) {
        receivedMethod = method;
        receivedParams = params;
    });

    QJsonObject notif;
    notif[QStringLiteral("jsonrpc")] = QStringLiteral("2.0");
    notif[QStringLiteral("method")] = QStringLiteral("notifications/test");
    QJsonObject params;
    params[QStringLiteral("value")] = 123;
    notif[QStringLiteral("params")] = params;
    m_transport->injectMessage(notif);

    QCOMPARE(receivedMethod, QStringLiteral("notifications/test"));
    QCOMPARE(receivedParams[QStringLiteral("value")].toInt(), 123);
}

void TestMCPClient::testInitialize()
{
    m_client->openConnection();

    bool callbackCalled = false;
    m_client->initialize(QStringLiteral("TestClient"), QStringLiteral("1.0"),
                          [&](bool success, const QJsonValue &, int, const QString &) {
        QVERIFY(success);
        callbackCalled = true;
    });

    QCOMPARE(m_transport->sentCount(), 1);
    QJsonObject initMsg = m_transport->sentAt(0);
    QCOMPARE(initMsg[QStringLiteral("method")].toString(), QStringLiteral("initialize"));

    // Send response
    QJsonObject response;
    response[QStringLiteral("jsonrpc")] = QStringLiteral("2.0");
    response[QStringLiteral("id")] = initMsg[QStringLiteral("id")];
    QJsonObject result;
    result[QStringLiteral("protocolVersion")] = QStringLiteral("2024-11-05");
    result[QStringLiteral("serverInfo")] = QJsonObject({
        {QStringLiteral("name"), QStringLiteral("Server")},
        {QStringLiteral("version"), QStringLiteral("1.0")}
    });
    result[QStringLiteral("capabilities")] = QJsonObject({{QStringLiteral("tools"), QJsonObject()}});
    response[QStringLiteral("result")] = result;
    m_transport->injectMessage(response);

    QVERIFY(callbackCalled);
}

void TestMCPClient::testPing()
{
    m_client->openConnection();

    bool callbackCalled = false;
    m_client->ping([&](bool success, const QJsonValue &, int, const QString &) {
        QVERIFY(success);
        callbackCalled = true;
    });

    // Send pong response
    QJsonObject pong;
    pong[QStringLiteral("jsonrpc")] = QStringLiteral("2.0");
    pong[QStringLiteral("id")] = m_transport->sentAt(0)[QStringLiteral("id")];
    pong[QStringLiteral("result")] = QJsonObject();
    m_transport->injectMessage(pong);

    QVERIFY(callbackCalled);
}

void TestMCPClient::testListTools()
{
    m_client->openConnection();

    m_client->listTools(nullptr);
    QCOMPARE(m_transport->sentCount(), 1);
    QCOMPARE(m_transport->sentAt(0)[QStringLiteral("method")].toString(), QStringLiteral("tools/list"));
}

void TestMCPClient::testCallTool()
{
    m_client->openConnection();

    QJsonObject args;
    args[QStringLiteral("input")] = QStringLiteral("hello");
    m_client->callTool(QStringLiteral("echo"), args, nullptr);
    QCOMPARE(m_transport->sentCount(), 1);

    QJsonObject msg = m_transport->sentAt(0);
    QCOMPARE(msg[QStringLiteral("method")].toString(), QStringLiteral("tools/call"));
    QCOMPARE(msg[QStringLiteral("params")].toObject()[QStringLiteral("name")].toString(), QStringLiteral("echo"));
    QCOMPARE(msg[QStringLiteral("params")].toObject()[QStringLiteral("arguments")].toObject()[QStringLiteral("input")].toString(), QStringLiteral("hello"));
}

void TestMCPClient::testListResources()
{
    m_client->openConnection();
    m_client->listResources(nullptr);
    QCOMPARE(m_transport->sentCount(), 1);
    QCOMPARE(m_transport->sentAt(0)[QStringLiteral("method")].toString(), QStringLiteral("resources/list"));
}

void TestMCPClient::testMultipleRequests()
{
    m_client->openConnection();

    int callbackCount = 0;
    m_client->sendRequest(QStringLiteral("req1"), QJsonObject(),
                           [&](bool, const QJsonValue &, int, const QString &) { callbackCount++; });
    m_client->sendRequest(QStringLiteral("req2"), QJsonObject(),
                           [&](bool, const QJsonValue &, int, const QString &) { callbackCount++; });
    m_client->sendRequest(QStringLiteral("req3"), QJsonObject(),
                           [&](bool, const QJsonValue &, int, const QString &) { callbackCount++; });

    QCOMPARE(m_transport->sentCount(), 3);

    // Send responses in reverse order to test id correlation
    for (int i = 2; i >= 0; --i) {
        QJsonObject response;
        response[QStringLiteral("jsonrpc")] = QStringLiteral("2.0");
        response[QStringLiteral("id")] = m_transport->sentAt(i)[QStringLiteral("id")];
        response[QStringLiteral("result")] = QJsonObject();
        m_transport->injectMessage(response);
    }

    QCOMPARE(callbackCount, 3);
}

QTEST_MAIN(TestMCPClient)
#include "tst_MCPClient.moc"
