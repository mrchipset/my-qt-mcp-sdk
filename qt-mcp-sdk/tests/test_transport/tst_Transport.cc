#include <QtTest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>
#include <QThread>
#include <QEventLoop>
#include <QTimer>

#include "MCPTransport.h"
#include "MCPStdioTransport.h"
#include "MCPHttpTransport.h"

//
// Test for MCPTransport hierarchy:
//   1. Mock transport (abstract interface test)
//   2. MCPStdioTransport (basic lifecycle)
//   3. MCPHttpTransport (basic lifecycle)
//

// ============================================================
// MockTransport — a transport that simulates message passing
// in-memory without any I/O. Used for testing MCPServer/MCPClient.
// ============================================================

class MockTransport : public MCPTransport
{
    Q_OBJECT
public:
    explicit MockTransport(QObject *parent = nullptr)
        : MCPTransport(parent) {}

    void open() override {
        m_open = true;
        emit opened();
    }
    void close() override {
        m_open = false;
        emit closed();
    }
    bool isOpen() const override { return m_open; }
    void sendMessage(const QJsonObject &message) override {
        m_lastSent = message;
        emit messageSent(message);
    }

    // Test helper: simulate receiving a message
    void injectMessage(const QJsonObject &message) {
        emit messageReceived(message);
    }

    // Test helper: get the last sent message
    QJsonObject lastSent() const { return m_lastSent; }

signals:
    void messageSent(const QJsonObject &message);

private:
    bool m_open = false;
    QJsonObject m_lastSent;
};

// ============================================================
// Test Class
// ============================================================

class TestTransport : public QObject
{
    Q_OBJECT

private slots:
    // MockTransport
    void testMockTransportLifecycle();
    void testMockTransportSendReceive();

    // MCPTransport base
    void testTransportBufferSize();

    // MCPStdioTransport
    void testStdioTransportLifecycle();

    // MCPHttpTransport
    void testHttpTransportLifecycle();
};

// ============================================================
// MockTransport Tests
// ============================================================

void TestTransport::testMockTransportLifecycle()
{
    MockTransport transport;
    QSignalSpy openedSpy(&transport, &MCPTransport::opened);
    QSignalSpy closedSpy(&transport, &MCPTransport::closed);

    QVERIFY(!transport.isOpen());

    transport.open();
    QVERIFY(transport.isOpen());
    QCOMPARE(openedSpy.count(), 1);

    transport.close();
    QVERIFY(!transport.isOpen());
    QCOMPARE(closedSpy.count(), 1);
}

void TestTransport::testMockTransportSendReceive()
{
    MockTransport transport;
    transport.open();

    // Test send
    QJsonObject msg;
    msg[QStringLiteral("test")] = true;
    transport.sendMessage(msg);
    QVERIFY(!transport.lastSent().isEmpty());
    QVERIFY(transport.lastSent()[QStringLiteral("test")].toBool());

    // Test receive via signal
    QSignalSpy msgSpy(&transport, &MCPTransport::messageReceived);
    QJsonObject incoming;
    incoming[QStringLiteral("hello")] = QStringLiteral("world");
    transport.injectMessage(incoming);
    QCOMPARE(msgSpy.count(), 1);
    QCOMPARE(msgSpy.at(0).at(0).toJsonObject()[QStringLiteral("hello")].toString(), QStringLiteral("world"));
}

// ============================================================
// MCPTransport base tests
// ============================================================

void TestTransport::testTransportBufferSize()
{
    MockTransport transport;
    QCOMPARE(transport.property("m_receiveBufferSize").isValid(), false); // private, but we test via setter
    transport.setReceiveBufferSize(2048);
    // Just verify no crash / error
    QVERIFY(true);
}

// ============================================================
// MCPStdioTransport Tests
// ============================================================

void TestTransport::testStdioTransportLifecycle()
{
    // Server mode should open without issues
    MCPStdioTransport transport(MCPStdioTransport::Mode::Server);
    QSignalSpy openedSpy(&transport, &MCPTransport::opened);

    QVERIFY(!transport.isOpen());
    transport.open();
    QVERIFY(transport.isOpen());
    QCOMPARE(openedSpy.count(), 1);

    transport.close();
    QVERIFY(!transport.isOpen());
}

// ============================================================
// MCPHttpTransport Tests
// ============================================================

void TestTransport::testHttpTransportLifecycle()
{
    // Test server mode
    MCPHttpTransport server(MCPHttpTransport::Mode::Server);
    server.setHost(QStringLiteral("127.0.0.1"), 0); // port 0 = auto-assign

    QSignalSpy openedSpy(&server, &MCPTransport::opened);
    QSignalSpy errorSpy(&server, &MCPTransport::errorOccurred);

    server.open();
    QVERIFY(server.isOpen());
    QCOMPARE(openedSpy.count(), 1);
    QCOMPARE(errorSpy.count(), 0);

    server.close();
    QVERIFY(!server.isOpen());

    // Test client mode
    MCPHttpTransport client(MCPHttpTransport::Mode::Client);
    client.setUrl(QUrl(QStringLiteral("http://127.0.0.1:9999/mcp")));

    QSignalSpy clientOpened(&client, &MCPTransport::opened);
    client.open();
    QVERIFY(client.isOpen());
    QCOMPARE(clientOpened.count(), 1);

    client.close();
}

QTEST_MAIN(TestTransport)
#include "tst_Transport.moc"
