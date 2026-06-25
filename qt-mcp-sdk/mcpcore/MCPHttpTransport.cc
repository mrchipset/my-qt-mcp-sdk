#include "MCPHttpTransport.h"

#include <QJsonDocument>
#include <QDebug>
#include <QUuid>

// ============================================================
// Constructor / Destructor
// ============================================================

MCPHttpTransport::MCPHttpTransport(Mode mode, QObject *parent)
    : MCPTransport(parent)
    , m_mode(mode)
{
    if (mode == Mode::Client) {
        m_networkManager = new QNetworkAccessManager(this);
    }
}

MCPHttpTransport::~MCPHttpTransport()
{
    close();
}

// ============================================================
// Configuration
// ============================================================

void MCPHttpTransport::setHost(const QString &host, quint16 port)
{
    m_host = host;
    m_port = port;
}

void MCPHttpTransport::setSslConfiguration(const QSslConfiguration &sslConfig)
{
    m_sslConfig = sslConfig;
    m_useSsl = true;
}

void MCPHttpTransport::setUrl(const QUrl &url)
{
    m_url = url;
}

void MCPHttpTransport::setSessionId(const QString &sessionId)
{
    m_sessionId = sessionId;
}

QString MCPHttpTransport::sessionId() const
{
    return m_sessionId;
}

// ============================================================
// Transport Interface
// ============================================================

void MCPHttpTransport::open()
{
    if (m_mode == Mode::Server) {
        if (m_server) {
            qWarning() << "MCPHttpTransport: server already running";
            return;
        }

        m_server = new QTcpServer(this);
        connect(m_server, &QTcpServer::newConnection, this, &MCPHttpTransport::onNewConnection);

        if (m_server->listen(QHostAddress(m_host), m_port)) {
            qDebug() << "MCPHttpTransport: server listening on" << m_host << ":" << m_port;
            emit opened();
            emit serverStarted();
        } else {
            QString err = QStringLiteral("Failed to listen on %1:%2 - %3")
                              .arg(m_host).arg(m_port).arg(m_server->errorString());
            qWarning() << err;
            emit errorOccurred(err);
        }
    } else {
        // Client mode: just mark as open, sendMessage will make HTTP requests
        emit opened();
        emit clientConnected();
    }
}

void MCPHttpTransport::close()
{
    if (m_server) {
        m_server->close();
        // Clean up remaining client sockets
        for (auto it = m_clientBuffers.begin(); it != m_clientBuffers.end(); ++it) {
            it.key()->close();
            it.key()->deleteLater();
        }
        m_clientBuffers.clear();
        m_clientSessionIds.clear();
        m_pendingResponses.clear();
        delete m_server;
        m_server = nullptr;
    }

    if (m_networkManager) {
        m_networkManager->deleteLater();
        m_networkManager = nullptr;
    }

    emit closed();
}

bool MCPHttpTransport::isOpen() const
{
    if (m_mode == Mode::Server)
        return m_server && m_server->isListening();
    return true; // Client mode is always "open" after calling open()
}

void MCPHttpTransport::sendMessage(const QJsonObject &message)
{
    QJsonDocument doc(message);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);

    if (m_mode == Mode::Server) {
        // MCPServer calls sendMessage synchronously during messageReceived signal emission.
        // m_currentRequestSocket is set by handleIncomingJson just before emitting.
        if (m_currentRequestSocket && m_currentRequestSocket->state() == QAbstractSocket::ConnectedState) {
            writeHttpResponse(m_currentRequestSocket, jsonData);
            m_currentRequestSocket = nullptr;
        } else {
            qWarning() << "MCPHttpTransport(Server): no active request socket to respond to";
        }
        return;
    }

    // Client mode: send HTTP POST
    if (m_url.isEmpty()) {
        emit errorOccurred(QStringLiteral("No URL configured for HTTP client transport"));
        return;
    }

    QNetworkRequest request(m_url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setRawHeader("Accept", "application/json");

    if (!m_sessionId.isEmpty()) {
        request.setRawHeader("Mcp-Session-Id", m_sessionId.toUtf8());
    }

    QNetworkReply *reply = m_networkManager->post(request, jsonData);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onReplyFinished(reply);
    });
}

// ============================================================
// Server Slots
// ============================================================

void MCPHttpTransport::onNewConnection()
{
    while (m_server->hasPendingConnections()) {
        QTcpSocket *socket = m_server->nextPendingConnection();
        m_clientBuffers[socket] = QByteArray();
        m_pendingResponses[socket] = QQueue<QByteArray>();
        // Generate a session id for this connection
        m_clientSessionIds[socket] = QUuid::createUuid().toString(QUuid::WithoutBraces);

        connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
            QByteArray incoming = socket->readAll();
            m_clientBuffers[socket].append(incoming);

            // Process as many complete requests as are in the buffer
            while (true) {
                QByteArray &buffer = m_clientBuffers[socket];
                int headerEnd = buffer.indexOf("\r\n\r\n");
                if (headerEnd < 0) {
                    // No complete HTTP header yet — if buffer is growing large without \r\n\r\n,
                    // something unexpected may have been received
                    if (buffer.size() > 4096) {
                        qWarning().noquote() << "MCPHttpTransport: buffer exceeds 4KB without finding HTTP header, dumping first 500 bytes:";
                        qWarning().noquote() << QString::fromUtf8(buffer.left(500).toHex(' '));
                        qWarning().noquote() << QString::fromUtf8(buffer.left(500));
                        buffer.clear();
                    }
                    break;
                }

                // Parse Content-Length from headers
                QByteArray header = buffer.left(headerEnd);
                int contentLength = 0;
                for (const QByteArray &line : header.split('\n')) {
                    if (line.trimmed().toLower().startsWith("content-length:")) {
                        contentLength = line.mid(line.indexOf(':') + 1).trimmed().toInt();
                        break;
                    }
                }

                int bodyStart = headerEnd + 4;
                int totalExpected = bodyStart + contentLength;

                if (buffer.size() < totalExpected)
                    break;

                // Skip requests with empty body (e.g. HTTP GET/HEAD, or NLA probes)
                if (contentLength == 0) {
                    QByteArray firstLine = header.left(header.indexOf('\n')).trimmed();
                    qDebug().noquote() << QStringLiteral("MCPHttpTransport: skipping %1 with Content-Length: 0")
                                              .arg(QString::fromUtf8(firstLine));
                    buffer.remove(0, totalExpected);
                    continue;
                }

                QByteArray body = buffer.mid(bodyStart, contentLength);
                buffer.remove(0, totalExpected);

                QString sessionId = m_clientSessionIds.value(socket);
                handleIncomingJson(body, socket, sessionId);
            }
        });

        connect(socket, &QTcpSocket::disconnected, this, [this, socket]() {
            m_clientBuffers.remove(socket);
            m_clientSessionIds.remove(socket);
            m_pendingResponses.remove(socket);
            socket->deleteLater();
        });
    }
}

// ============================================================
// Client Slots
// ============================================================

void MCPHttpTransport::onReplyFinished(QNetworkReply *reply)
{
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        QString errMsg = QStringLiteral("HTTP request failed: %1").arg(reply->errorString());
        qWarning() << errMsg;
        emit errorOccurred(errMsg);
        return;
    }

    // Check for session id header
    if (reply->hasRawHeader("Mcp-Session-Id")) {
        m_sessionId = QString::fromUtf8(reply->rawHeader("Mcp-Session-Id"));
    }

    QByteArray responseData = reply->readAll();
    if (responseData.isEmpty())
        return;

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(responseData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        qWarning().noquote() << QStringLiteral("MCPHttpTransport(Client): JSON parse error (%1), raw data (%2 bytes):")
                                    .arg(parseError.errorString())
                                    .arg(responseData.size());
        qWarning().noquote() << QStringLiteral("  hex:") << QString::fromUtf8(responseData.toHex(' '));
        qWarning().noquote() << QStringLiteral("  text:") << QString::fromUtf8(responseData);
        return;
    }

    if (!doc.isObject()) {
        qWarning() << "MCPHttpTransport: received non-object JSON";
        return;
    }

    emit messageReceived(doc.object());
}

// ============================================================
// Internal
// ============================================================

void MCPHttpTransport::handleIncomingJson(const QByteArray &data, QTcpSocket *socket, const QString &sessionId)
{
    Q_UNUSED(sessionId)

    if (data.isEmpty()) {
        qWarning() << "MCPHttpTransport: received empty body, ignoring";
        return;
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        qWarning().noquote() << QStringLiteral("MCPHttpTransport: JSON parse error (%1), raw data (%2 bytes):")
                                    .arg(parseError.errorString())
                                    .arg(data.size());
        qWarning().noquote() << QStringLiteral("  hex:") << QString::fromUtf8(data.toHex(' '));
        qWarning().noquote() << QStringLiteral("  text:") << QString::fromUtf8(data);
        // Send back an error response
        QJsonObject errResp;
        errResp[QStringLiteral("jsonrpc")] = QStringLiteral("2.0");
        errResp[QStringLiteral("id")] = QJsonValue::Null;
        QJsonObject error;
        error[QStringLiteral("code")] = -32700;
        error[QStringLiteral("message")] = QStringLiteral("Parse error");
        errResp[QStringLiteral("error")] = error;
        QByteArray jsonErr = QJsonDocument(errResp).toJson(QJsonDocument::Compact);
        writeHttpResponse(socket, jsonErr);
        return;
    }

    if (!doc.isObject()) {
        qWarning() << "MCPHttpTransport: received non-object JSON";
        return;
    }

    // Set the current socket so sendMessage (called synchronously by MCPServer)
    // knows where to write the HTTP response.
    m_currentRequestSocket = socket;
    emit messageReceived(doc.object());
    // After the synchronous signal handling, clear the reference.
    // If sendMessage was not called (e.g. notification), we still clear it.
    m_currentRequestSocket = nullptr;
}

void MCPHttpTransport::writeHttpResponse(QTcpSocket *socket, const QByteArray &jsonBody)
{
    QByteArray response;
    response.append("HTTP/1.1 200 OK\r\n");
    response.append("Content-Type: application/json\r\n");
    response.append("Content-Length: " + QByteArray::number(jsonBody.size()) + "\r\n");
    response.append("Connection: keep-alive\r\n");
    response.append("Access-Control-Allow-Origin: *\r\n");
    response.append("\r\n");
    response.append(jsonBody);

    socket->write(response);
    socket->flush();
}

void MCPHttpTransport::flushPendingResponses(QTcpSocket *socket)
{
    auto it = m_pendingResponses.find(socket);
    if (it == m_pendingResponses.end())
        return;

    while (!it.value().isEmpty()) {
        QByteArray jsonBody = it.value().dequeue();
        writeHttpResponse(socket, jsonBody);
    }
}
