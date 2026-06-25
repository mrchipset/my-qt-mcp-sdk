# Qt MCP SDK

[![License: LGPL v2.1](https://img.shields.io/badge/License-LGPL%20v2.1-blue.svg)](LICENSE)
![Qt 5.15](https://img.shields.io/badge/Qt-5.15-green.svg)
![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)

A **Qt5-based MCP (Model Context Protocol) SDK** providing client and server libraries for integrating MCP protocol communication into Qt applications.

The **Model Context Protocol (MCP)** is an open protocol that standardizes how applications provide context and tools to Large Language Models (LLMs). This SDK enables Qt applications to act as MCP servers (exposing tools/resources/prompts to LLM clients) or MCP clients (connecting to MCP servers).

---

## Table of Contents

- [Architecture](#architecture)
- [Modules](#modules)
  - [mcpcore — Core Library](#mcpcore--core-library)
  - [mcpclient — Client Library](#mcpclient--client-library)
  - [mcpserver — Server Library](#mcpserver--server-library)
- [Quick Start](#quick-start)
- [Examples](#examples)
  - [Simple Server](#simple-server)
  - [Simple Client](#simple-client)
- [Build](#build)
  - [Prerequisites](#prerequisites)
  - [Shadow Build (Recommended)](#shadow-build-recommended)
  - [Build Output Layout](#build-output-layout)
- [Dependency Graph](#dependency-graph)
- [Testing](#testing)
- [Contributing](#contributing)
- [License](#license)

---

## Architecture

The SDK is organized into three layered libraries plus examples and tests:

```
┌──────────────────────────────┐
│      examples + tests        │
├──────────────┬───────────────┤
│  mcpclient   │   mcpserver   │  ← Client / Server libraries
├──────────────┴───────────────┤
│          mcpcore              │  ← Core protocol types & transports
└──────────────────────────────┘
```

- **mcpcore** — Foundation layer with no external dependencies beyond Qt. Provides MCP data structures, JSON-RPC 2.0 serialization, and abstract transport interfaces.
- **mcpclient** — Full MCP client implementation built on mcpcore.
- **mcpserver** — Full MCP server implementation built on mcpcore.

---

## Modules

### mcpcore — Core Library

The `mcpcore` module provides shared base components used by both client and server libraries.

#### Data Structures (`MCPTypes.h`)

JSON-RPC 2.0 message types:
| Struct | Description |
|--------|-------------|
| `MCPRequest` | JSON-RPC request with id, method, and params |
| `MCPResponse` | JSON-RPC response with id and result |
| `MCPErrorResponse` | JSON-RPC error response with error code and message |
| `MCPNotification` | JSON-RPC notification (no response expected) |

MCP content types:
| Struct | Description |
|--------|-------------|
| `MCPTextContent` | Text content (type: "text") |
| `MCPImageContent` | Base64-encoded image content |
| `MCPResourceContent` | Resource content with URI and MIME type |

MCP protocol objects:
| Struct | Description |
|--------|-------------|
| `MCPTool` | Tool definition with name, description, and JSON Schema |
| `MCPResource` | Resource definition with URI and metadata |
| `MCPPrompt` | Prompt template definition |

All types support `toJson()` / `fromJson()` serialization.

#### JSON-RPC Serializer (`MCPJsonRpcSerializer`)

Handles serialization and deserialization of JSON-RPC 2.0 messages:

```cpp
// Serialize a request
QJsonObject json = MCPJsonRpcSerializer::serializeRequest(request);

// Parse incoming message
QString type = MCPJsonRpcSerializer::identifyMessage(json);
if (type == "request") {
    MCPRequest req = MCPJsonRpcSerializer::parseRequest(json);
}
```

#### Transport Layer (`MCPTransport`)

Abstract base class defining the transport interface:

| Method | Description |
|--------|-------------|
| `open()` / `close()` | Connection lifecycle |
| `isOpen()` | Connection state |
| `sendMessage()` | Send a JSON-RPC message |
| `messageReceived` (signal) | Emitted when a complete message is received |

##### Built-in Transport Implementations

**MCPStdioTransport** — Communication via stdin/stdout:
- **Server mode**: reads from stdin, writes to stdout (classic MCP stdio mode)
- **Client mode**: spawns a child process and communicates via its stdin/stdout

```cpp
// Client mode: spawn a child process
auto transport = std::make_unique<MCPStdioTransport>(MCPStdioTransport::Mode::Client);
transport->setCommand("path/to/mcp-server", {"--arg1"});

// Server mode: communicate via stdin/stdout
auto transport = std::make_unique<MCPStdioTransport>(MCPStdioTransport::Mode::Server);
```

**MCPHttpTransport** — Communication via HTTP(S):
- **Server mode**: listens on a TCP port for HTTP POST requests
- **Client mode**: sends HTTP POST requests to a remote MCP server
- Supports SSL/TLS and SSE-based server push notifications

```cpp
// Client mode
auto transport = std::make_unique<MCPHttpTransport>(MCPHttpTransport::Mode::Client);
transport->setUrl(QUrl("http://localhost:8080/mcp"));

// Server mode
auto transport = std::make_unique<MCPHttpTransport>(MCPHttpTransport::Mode::Server);
transport->setHost("0.0.0.0", 8080);
```

---

### mcpclient — Client Library

`MCPClient` is the main class for connecting to MCP servers. Built on `QObject` with full signal/slot support.

#### Lifecycle

```cpp
auto transport = std::make_unique<MCPStdioTransport>(/*...*/);
MCPClient client(std::move(transport));

// Connect signals
QObject::connect(&client, &MCPClient::connected, []() {
    qDebug() << "Connected to MCP server!";
});
QObject::connect(&client, &MCPClient::errorOccurred, [](const QString &err) {
    qDebug() << "Error:" << err;
});

// Open connection and initialize
client.openConnection();
client.initialize("MyApp", "1.0.0", [](bool ok, const QJsonValue &result, ...) {
    if (ok) qDebug() << "Initialized!";
});
```

#### API Methods

| Method | Description |
|--------|-------------|
| `initialize()` | MCP handshake with server info |
| `sendRequest()` | Send any JSON-RPC request with callback |
| `sendNotification()` | Send a one-way notification |
| `ping()` | Ping the server |
| `listTools()` | List available tools |
| `callTool()` | Invoke a tool by name |
| `listResources()` / `readResource()` | Browse and read resources |
| `listPrompts()` / `getPrompt()` | Browse and get prompts |
| `setNotificationHandler()` | Handle server→client notifications |

#### Signals

| Signal | Description |
|--------|-------------|
| `connected()` / `disconnected()` | Connection state changes |
| `initialized()` | Handshake completed |
| `errorOccurred()` | Transport or protocol errors |

---

### mcpserver — Server Library

`MCPServer` is the main class for building MCP servers. Exposes tools, resources, and prompts to MCP clients.

#### Lifecycle

```cpp
auto transport = std::make_unique<MCPStdioTransport>(MCPStdioTransport::Mode::Server);
MCPServer server(std::move(transport));

// Register a tool
server.registerTool("greet", "Greet a user",
    R"({"type":"object","properties":{"name":{"type":"string"}}})"_json,
    [](const QJsonObject &params) -> QJsonObject {
        return {{"content", QJsonArray{
            QJsonObject{{"type", "text"}, {"text", "Hello, " + params["name"].toString() + "!"}}
        }}};
    });

server.setServerInfo("MyServer", "1.0.0");
server.setServerCapabilities(true, false, false, false);
server.start();
```

#### Tool Registration

```cpp
server.registerTool(
    "tool-name",                                    // Tool name
    "Description of what this tool does",           // Description
    inputSchema,                                    // JSON Schema for params
    handler                                         // Handler function
);
server.removeTool("tool-name");
server.toolNames();                                 // List all tools
```

#### Resource Registration

```cpp
server.registerResource(
    "file:///docs/readme",                          // Resource URI
    "README",                                       // Display name
    "Project readme document",                      // Description
    "text/markdown",                                // MIME type
    [](const QString &uri) -> QJsonObject {         // Reader callback
        return {{"contents", QJsonArray{/*...*/}}};
    }
);
```

#### Prompt Registration

```cpp
server.registerPrompt(
    "analyze-code",                                 // Prompt name
    "Analyze source code",                          // Description
    argumentsSchema,                                // Arguments JSON Schema
    handler                                         // Handler callback
);
```

#### Server Configuration

```cpp
server.setServerInfo("MyServer", "1.0.0");
server.setServerCapabilities(
    /*tools=*/true,      // Supports tool calling
    /*resources=*/true,  // Supports resource access
    /*prompts=*/true,    // Supports prompt templates
    /*logging=*/false    // Supports logging
);
```

#### Signals

| Signal | Description |
|--------|-------------|
| `started()` / `stopped()` | Server state changes |
| `errorOccurred()` | Server errors |
| `clientConnected()` / `clientDisconnected()` | Client connection events |
| `toolCalled()` | Tool invocation monitoring |

---

## Quick Start

Here's a minimal MCP server that runs in stdio mode:

```cpp
#include <QCoreApplication>
#include <MCPServer.h>
#include <MCPStdioTransport.h>

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    auto transport = std::make_unique<MCPStdioTransport>(
        MCPStdioTransport::Mode::Server);
    MCPServer server(std::move(transport));

    server.registerTool("hello", "Say hello",
        QJsonObject{{"type", "object"}, {"properties", QJsonObject{}}},
        [](const QJsonObject &) -> QJsonObject {
            return {{"content", QJsonArray{
                QJsonObject{{"type", "text"}, {"text", "Hello from Qt MCP SDK!"}}
            }}};
        });

    server.setServerInfo("HelloServer", "1.0.0");
    server.setServerCapabilities(true, false, false, false);
    server.start();

    return app.exec();
}
```

---

## Examples

### Simple Server

Located in `qt-mcp-sdk/examples/simpleserver/`, this is a Qt Widgets-based MCP server demo:

- **GUI mode**: A main window with mode selection (stdio/HTTP), host/port configuration, and start/stop controls. The server window stays visible for monitoring.
- **CLI mode**: Run with `--stdio` flag to start in headless stdio mode (no GUI).

The server registers two MCP tools:
- `set_text` — Sets the label text in the GUI (demonstrates server→UI interaction)
- `get_text` — Reads the current label text

Launch with:
```bash
# Interactive GUI mode
simpleserver.exe

# Headless stdio mode (for LLM client integration)
simpleserver.exe --stdio
```

### Simple Client

Located in `qt-mcp-sdk/examples/simpleclient/`, this is a Qt Widgets-based MCP client demo that connects to an MCP server and allows interactive tool calls through a graphical interface.

---

## Build

### Prerequisites

- **Qt 5.15** (tested with MSVC 2019 64-bit)
- **C++17** compatible compiler (MSVC, GCC, Clang)
- **qmake** build system
- Windows: Visual Studio 2022 (or equivalent with C++ tools)

### Shadow Build (Recommended)

All build artifacts are kept in the `build/` directory, keeping source directories completely clean.

```bash
# 1. Generate Makefiles (Debug)
cd build
qmake ../qt-mcp-sdk/qt-mcp-sdk.pro -spec win32-msvc "CONFIG+=debug"

# 2. Compile all modules
nmake

# Full cleanup
nmake distclean
```

> **Note**: On Windows, ensure the Visual Studio environment is initialized first:
> ```cmd
> "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
> ```

### Available Build Targets

| Command | Description |
|---------|-------------|
| `qmake <pro>` | Generate Makefiles (run once or after .pro changes) |
| `nmake` | Build all modules |
| `nmake distclean` | Clean all generated files |
| `qmake CONFIG+=release` | Generate Release Makefiles |

### Build Output Layout

```
build/
├── bin/                        # ← Unified output directory
│   ├── mcpcore.dll / .lib      #   Core library
│   ├── mcpclient.dll / .lib    #   Client library
│   ├── mcpserver.dll / .lib    #   Server library
│   ├── simpleserver.exe        #   Example server
│   ├── simpleclient.exe        #   Example client
│   └── test_*.exe              #   Test executables
├── intermediates/              # Object files, MOC outputs, UI headers
│   ├── mcpcore/
│   ├── mcpclient/
│   ├── mcpserver/
│   ├── simpleserver/
│   └── simpleclient/
├── mcpcore/                    # Generated Makefiles
├── mcpclient/
├── mcpserver/
├── examples/
└── tests/
```

All `.dll` and `.exe` files are unified under `build/bin/`, eliminating runtime dependency copy issues.

---

## Dependency Graph

```
simpleserver (exe) ──┐
                      ├── mcpclient (dll) ──┐
                      └── mcpserver (dll) ──┤
                                             └── mcpcore (dll)
                                                      └── Qt5 (Core, Network, Widgets)
```

**Build order**: `mcpcore` → `mcpclient` + `mcpserver` (parallel) → `examples` → `tests`

---

## Testing

The SDK includes unit tests located in `qt-mcp-sdk/tests/`. Tests are built automatically as part of the build process.

| Test | Description |
|------|-------------|
| `test_mcptypes` | Tests for MCP data structures serialization |
| `test_jsonrpc` | Tests for JSON-RPC 2.0 message parsing |
| `test_transport` | Tests for stdio and HTTP transport layers |
| `test_server` | Integration tests for MCPServer |
| `test_client` | Integration tests for MCPClient |

Run all tests after building:
```bash
cd build/bin
test_mcptypes.exe
test_jsonrpc.exe
test_transport.exe
test_server.exe
test_client.exe
```

---

## Project Structure

```
my-qt-mcp-sdk/
├── AGENTS.md                   # AI agent development guide
├── qt-mcp-sdk/                 # Qt subdirs project root
│   ├── qt-mcp-sdk.pro          # Top-level project file
│   ├── common.pri              # Shared build configuration
│   ├── mcpcore/                # Core library sources
│   │   ├── MCPCore.h/.cc       # Core class
│   │   ├── MCPTypes.h/.cc      # Protocol data structures
│   │   ├── MCPTransport.h/.cc  # Transport abstract interface
│   │   ├── MCPStdioTransport.h/.cc  # STDIO transport
│   │   ├── MCPHttpTransport.h/.cc   # HTTP transport
│   │   ├── MCPJsonRpcSerializer.h/.cc # JSON-RPC serializer
│   │   └── mcpcore_global.h    # Export macros
│   ├── mcpclient/              # Client library sources
│   │   ├── MCPClient.h/.cc     # Client implementation
│   │   └── mcpclient_global.h
│   ├── mcpserver/              # Server library sources
│   │   ├── MCPServer.h/.cc     # Server implementation
│   │   └── mcpserver_global.h
│   ├── examples/               # Example applications
│   │   ├── simpleserver/       # GUI + CLI MCP server demo
│   │   └── simpleclient/       # GUI MCP client demo
│   └── tests/                  # Unit and integration tests
└── build/                      # Build output (clean shadow build)
```

---

## License

This project is licensed under the **GNU Lesser General Public License v2.1** — see the [LICENSE](LICENSE) file for details.
