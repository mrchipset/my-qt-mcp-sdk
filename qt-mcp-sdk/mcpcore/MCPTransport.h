#ifndef MCPTRANSPORT_H
#define MCPTRANSPORT_H

#include "mcpcore_global.h"

#include <QObject>
#include <QJsonObject>
#include <QString>

//
// MCPTransport
//
// Abstract interface for MCP transport layers.
// Supports both stdio and HTTP(s) communication patterns.
//

class MCPCORE_EXPORT MCPTransport : public QObject
{
    Q_OBJECT

public:
    explicit MCPTransport(QObject *parent = nullptr);
    virtual ~MCPTransport();

    // Lifecycle
    virtual void open() = 0;
    virtual void close() = 0;
    virtual bool isOpen() const = 0;

    // Send a JSON-RPC message
    virtual void sendMessage(const QJsonObject &message) = 0;

    // Receive buffer (for stream-based transports)
    // Called by the transport when a complete JSON message is received.
    void setReceiveBufferSize(int size);

signals:
    // Emitted when a complete JSON-RPC message is received
    void messageReceived(const QJsonObject &message);

    // Connection state signals
    void opened();
    void closed();
    void errorOccurred(const QString &errorMessage);

protected:
    int m_receiveBufferSize = 1024 * 1024; // 1MB default
};

#endif // MCPTRANSPORT_H
