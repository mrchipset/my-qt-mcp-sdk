#include "MainWindow.h"
#include "ui_MainWindow.h"

#include "MCPClient.h"
#include "MCPStdioTransport.h"
#include "MCPHttpTransport.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QDateTime>
#include <QMessageBox>
#include <QFileInfo>
#include <QFileDialog>
#include <QCoreApplication>
#include <QDir>

// ============================================================
// Constructor / Destructor
// ============================================================

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setWindowTitle(QStringLiteral("Qt MCP Simple Client"));

    // Connect UI buttons
    connect(ui->connectBtn, &QPushButton::clicked, this, &MainWindow::onConnectClicked);
    connect(ui->disconnectBtn, &QPushButton::clicked, this, &MainWindow::onDisconnectClicked);
    connect(ui->listToolsBtn, &QPushButton::clicked, this, &MainWindow::onListToolsClicked);
    connect(ui->setTextBtn, &QPushButton::clicked, this, &MainWindow::onSetTextClicked);
    connect(ui->getTextBtn, &QPushButton::clicked, this, &MainWindow::onGetTextClicked);
    connect(ui->pingBtn, &QPushButton::clicked, this, &MainWindow::onPingClicked);

    // Browse button
    connect(ui->browseBtn, &QPushButton::clicked, this, [this]() {
        QString path = QFileDialog::getOpenFileName(this,
            QStringLiteral("Select Server Executable"),
            QString(),
            QStringLiteral("Executables (*.exe);;All Files (*)"));
        if (!path.isEmpty())
            ui->serverPathEdit->setText(path);
    });

    // Initial state: disconnected
    setConnectedState(false);
}

MainWindow::~MainWindow()
{
    delete ui;
}

// ============================================================
// Connection
// ============================================================

void MainWindow::onConnectClicked()
{
    if (m_client) {
        appendLog(QStringLiteral("Already connected. Disconnect first."));
        return;
    }

    int modeIndex = ui->modeCombo->currentIndex(); // 0=stdio, 1=http
    std::unique_ptr<MCPTransport> transport;

    if (modeIndex == 0) {
        // ---- Stdio mode ----
        QString serverPath = ui->serverPathEdit->text().trimmed();
        if (serverPath.isEmpty()) {
            showError(QStringLiteral("Please specify the server executable path for stdio mode."));
            return;
        }

        // Resolve relative path against the binary directory
        QFileInfo fi(serverPath);
        if (fi.isRelative()) {
            QString baseDir = QCoreApplication::applicationDirPath();
            serverPath = QDir(baseDir).absoluteFilePath(serverPath);
        }

        if (!QFileInfo::exists(serverPath)) {
            showError(QStringLiteral("Server executable not found: ") + serverPath);
            return;
        }

        auto stdioTransport = std::make_unique<MCPStdioTransport>(MCPStdioTransport::Mode::Client);
        stdioTransport->setCommand(serverPath, {QStringLiteral("--stdio")});
        transport = std::move(stdioTransport);
        appendLog(QStringLiteral("Connecting via stdio to: ") + serverPath + QStringLiteral(" --stdio"));
    } else {
        // ---- HTTP mode ----
        QString host = ui->httpHostEdit->text().trimmed();
        if (host.isEmpty())
            host = QStringLiteral("127.0.0.1");
        int port = ui->httpPortSpin->value();

        QUrl url;
        url.setScheme(QStringLiteral("http"));
        url.setHost(host);
        url.setPort(port);

        auto httpTransport = std::make_unique<MCPHttpTransport>(MCPHttpTransport::Mode::Client);
        httpTransport->setUrl(url);
        transport = std::move(httpTransport);
        appendLog(QStringLiteral("Connecting via HTTP to: ") + url.toString());
    }

    // ---- Create client ----
    m_client = std::make_unique<MCPClient>(std::move(transport), this);

    // Connect signals
    connect(m_client.get(), &MCPClient::connected, this, &MainWindow::onClientConnected);
    connect(m_client.get(), &MCPClient::disconnected, this, &MainWindow::onClientDisconnected);
    connect(m_client.get(), &MCPClient::errorOccurred, this, &MainWindow::onClientError);
    connect(m_client.get(), &MCPClient::initialized, this, &MainWindow::onClientInitialized);

    // Set notification handler
    m_client->setNotificationHandler([this](const QString &method, const QJsonObject &params) {
        appendLog(QStringLiteral("Notification received: ") + method);
        if (!params.isEmpty())
            appendJson(QStringLiteral("notification params"), params);
    });

    m_client->openConnection();
    ui->connectBtn->setEnabled(false);
    appendLog(QStringLiteral("Opening connection..."));
}

void MainWindow::onDisconnectClicked()
{
    if (m_client) {
        m_client->closeConnection();
        m_client.reset();
    }
    setConnectedState(false);
    appendLog(QStringLiteral("Disconnected."));
}

// ============================================================
// Client signal handlers
// ============================================================

void MainWindow::onClientConnected()
{
    appendLog(QStringLiteral("--- Transport connected ---"));
    appendLog(QStringLiteral("Sending initialize request..."));

    // Automatically send initialize after connected
    m_client->initialize(QStringLiteral("QtSimpleClient"), QStringLiteral("1.0.0"),
        [this](bool success, const QJsonValue &result, int code, const QString &msg) {
            if (!success) {
                appendLog(QStringLiteral("Initialize failed: [%1] %2").arg(code).arg(msg));
                return;
            }
            appendJson(QStringLiteral("initialize result"), result);

            // Send initialized notification
            m_client->sendNotification(QStringLiteral("notifications/initialized"));
            appendLog(QStringLiteral("--- Ready. You can now send requests. ---"));
        });
}

void MainWindow::onClientDisconnected()
{
    setConnectedState(false);
    appendLog(QStringLiteral("--- Transport disconnected ---"));
}

void MainWindow::onClientError(const QString &errorMessage)
{
    appendLog(QStringLiteral("ERROR: ") + errorMessage);
}

void MainWindow::onClientInitialized()
{
    appendLog(QStringLiteral("--- Client initialized ---"));
    setConnectedState(true);
}

// ============================================================
// Request buttons
// ============================================================

void MainWindow::onListToolsClicked()
{
    if (!m_client) return;

    appendLog(QStringLiteral("Sending tools/list..."));

    m_client->listTools([this](bool ok, const QJsonValue &result, int code, const QString &msg) {
        if (!ok) {
            appendLog(QStringLiteral("tools/list failed: [%1] %2").arg(code).arg(msg));
            return;
        }
        appendJson(QStringLiteral("tools/list"), result);
    });
}

void MainWindow::onSetTextClicked()
{
    if (!m_client) return;

    QString text = ui->setTextEdit->text().trimmed();
    if (text.isEmpty()) {
        showError(QStringLiteral("Please enter text to set."));
        return;
    }

    appendLog(QStringLiteral("Sending tools/call set_text..."));

    QJsonObject args;
    args[QStringLiteral("text")] = text;

    m_client->callTool(QStringLiteral("set_text"), args,
        [this, text](bool ok, const QJsonValue &result, int code, const QString &msg) {
            if (!ok) {
                appendLog(QStringLiteral("set_text failed: [%1] %2").arg(code).arg(msg));
                return;
            }
            appendJson(QStringLiteral("set_text result"), result);
        });
}

void MainWindow::onGetTextClicked()
{
    if (!m_client) return;

    appendLog(QStringLiteral("Sending tools/call get_text..."));

    m_client->callTool(QStringLiteral("get_text"), QJsonObject(),
        [this](bool ok, const QJsonValue &result, int code, const QString &msg) {
            if (!ok) {
                appendLog(QStringLiteral("get_text failed: [%1] %2").arg(code).arg(msg));
                return;
            }
            appendJson(QStringLiteral("get_text result"), result);
        });
}

void MainWindow::onPingClicked()
{
    if (!m_client) return;

    appendLog(QStringLiteral("Sending ping..."));

    m_client->ping([this](bool ok, const QJsonValue &, int code, const QString &msg) {
        if (ok) {
            appendLog(QStringLiteral("Ping OK"));
        } else {
            appendLog(QStringLiteral("Ping failed: [%1] %2").arg(code).arg(msg));
        }
    });
}

// ============================================================
// Helpers
// ============================================================

void MainWindow::appendLog(const QString &message)
{
    QString timestamp = QDateTime::currentDateTime().toString(QStringLiteral("hh:mm:ss.zzz"));
    ui->logText->append(QStringLiteral("[%1] %2").arg(timestamp, message));
}

void MainWindow::appendJson(const QString &label, const QJsonValue &val)
{
    QJsonDocument doc;
    if (val.isObject())
        doc = QJsonDocument(val.toObject());
    else if (val.isArray())
        doc = QJsonDocument(val.toArray());
    else
        doc = QJsonDocument(QJsonObject{{QStringLiteral("value"), val}});

    appendLog(QStringLiteral("--- %1 ---").arg(label));
    ui->logText->append(QString::fromUtf8(doc.toJson(QJsonDocument::Indented)));
}

void MainWindow::setConnectedState(bool connected)
{
    ui->connectBtn->setEnabled(!connected);
    ui->disconnectBtn->setEnabled(connected);
    ui->listToolsBtn->setEnabled(connected);
    ui->setTextBtn->setEnabled(connected);
    ui->getTextBtn->setEnabled(connected);
    ui->pingBtn->setEnabled(connected);

    // Disable config widgets when connected
    ui->modeCombo->setEnabled(!connected);
    ui->serverPathEdit->setEnabled(!connected);
    ui->httpHostEdit->setEnabled(!connected);
    ui->httpPortSpin->setEnabled(!connected);
    ui->browseBtn->setEnabled(!connected);
}

void MainWindow::showError(const QString &msg)
{
    QMessageBox::warning(this, QStringLiteral("Error"), msg);
}
