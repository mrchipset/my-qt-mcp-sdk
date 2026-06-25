#include "MCPTransport.h"

MCPTransport::MCPTransport(QObject *parent)
    : QObject(parent)
{
}

MCPTransport::~MCPTransport() = default;

void MCPTransport::setReceiveBufferSize(int size)
{
    m_receiveBufferSize = size;
}
