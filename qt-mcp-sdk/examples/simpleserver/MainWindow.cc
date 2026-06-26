#include "MainWindow.h"
#include "ui_MainWindow.h"

#include "MCPServer.h"
#include "MCPStdioTransport.h"
#include "MCPHttpTransport.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMessageBox>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QStatusBar>
#include <QTimer>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent, bool autoStartStdio)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setWindowTitle(QStringLiteral("Qt MCP Simple Server"));

    // --- Mode selection row ---
    QHBoxLayout *modeLayout = new QHBoxLayout();

    QLabel *modeLabel = new QLabel(QStringLiteral("Transport:"), this);
    m_modeCombo = new QComboBox(this);
    m_modeCombo->setObjectName(QStringLiteral("modeCombo"));
    m_modeCombo->addItem(QStringLiteral("stdio"));
    m_modeCombo->addItem(QStringLiteral("HTTP"));

    QLabel *hostLabel = new QLabel(QStringLiteral("Host:"), this);
    m_hostEdit = new QLineEdit(this);
    m_hostEdit->setObjectName(QStringLiteral("hostEdit"));
    m_hostEdit->setText(QStringLiteral("127.0.0.1"));
    m_hostEdit->setEnabled(false); // disabled for stdio
    m_hostEdit->setPlaceholderText(QStringLiteral("e.g. 127.0.0.1"));

    QLabel *portLabel = new QLabel(QStringLiteral("Port:"), this);
    m_portSpin = new QSpinBox(this);
    m_portSpin->setObjectName(QStringLiteral("portSpin"));
    m_portSpin->setRange(1024, 65535);
    m_portSpin->setValue(8081);
    m_portSpin->setEnabled(false); // disabled for stdio

    connect(m_modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index) {
        bool isHttp = (index == 1);
        m_hostEdit->setEnabled(isHttp);
        m_portSpin->setEnabled(isHttp);
    });

    modeLayout->addWidget(modeLabel);
    modeLayout->addWidget(m_modeCombo);
    modeLayout->addWidget(hostLabel);
    modeLayout->addWidget(m_hostEdit);
    modeLayout->addWidget(portLabel);
    modeLayout->addWidget(m_portSpin);

    // --- Add widgets to layout ---
    ui->verticalLayout->addLayout(modeLayout);

    // Start/Stop button
    m_toggleBtn = new QPushButton(QStringLiteral("Start MCP Server"), this);
    m_toggleBtn->setObjectName(QStringLiteral("toggleBtn"));
    connect(m_toggleBtn, &QPushButton::clicked, this, &MainWindow::onStartStopClicked);
    ui->verticalLayout->addWidget(m_toggleBtn);

    // Status info
    m_statusLabel = new QLabel(QStringLiteral("MCP Server: Stopped"), this);
    m_statusLabel->setObjectName(QStringLiteral("statusLabel"));
    ui->verticalLayout->addWidget(m_statusLabel);

    ui->verticalLayout->addStretch();

    // If --stdio was passed on command line, auto-start after event loop starts
    if (autoStartStdio) {
        QTimer::singleShot(0, this, [this]() {
            startStdioServer();
        });
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::startStdioServer()
{
    if (m_serverRunning)
        return;

    // Set mode combo to stdio and disable it
    m_modeCombo->setCurrentIndex(0);
    m_modeCombo->setEnabled(false);
    m_hostEdit->setEnabled(false);
    m_portSpin->setEnabled(false);

    // Stdio transport
    auto stdioTransport = std::make_unique<MCPStdioTransport>(MCPStdioTransport::Mode::Server);
    m_mcpServer = std::make_unique<MCPServer>(std::move(stdioTransport), this);

    setupToolsAndStart(QStringLiteral("stdio"));
}

void MainWindow::onStartStopClicked()
{
    if (m_serverRunning) {
        // ---- Stop ----
        if (m_mcpServer) {
            m_mcpServer->stop();
        }
        m_mcpServer.reset();
        m_serverRunning = false;

        m_modeCombo->setEnabled(true);
        m_hostEdit->setEnabled(m_modeCombo->currentIndex() == 1);
        m_portSpin->setEnabled(m_modeCombo->currentIndex() == 1);
        m_toggleBtn->setText(QStringLiteral("Start MCP Server"));
        m_statusLabel->setText(QStringLiteral("MCP Server: Stopped"));
        statusBar()->showMessage(QStringLiteral("MCP Server stopped"), 3000);
    } else {
        // ---- Start ----
        m_modeCombo->setEnabled(false);
        m_hostEdit->setEnabled(false);
        m_portSpin->setEnabled(false);

        bool isHttp = (m_modeCombo->currentIndex() == 1);
        QString modeStr;

        if (isHttp) {
            QString host = m_hostEdit->text().trimmed();
            if (host.isEmpty()) {
                host = QStringLiteral("127.0.0.1");
            }
            auto httpTransport = std::make_unique<MCPHttpTransport>(MCPHttpTransport::Mode::Server);
            httpTransport->setHost(host, static_cast<quint16>(m_portSpin->value()));
            m_mcpServer = std::make_unique<MCPServer>(std::move(httpTransport), this);
            modeStr = QStringLiteral("http://%1:%2").arg(host).arg(m_portSpin->value());
        } else {
            auto stdioTransport = std::make_unique<MCPStdioTransport>(MCPStdioTransport::Mode::Server);
            m_mcpServer = std::make_unique<MCPServer>(std::move(stdioTransport), this);
            modeStr = QStringLiteral("stdio");
        }

        setupToolsAndStart(modeStr);
    }
}

void MainWindow::setupToolsAndStart(const QString &modeStr)
{
    m_mcpServer->setServerInfo(QStringLiteral("QtSimpleServer"), QStringLiteral("1.0.0"));
    m_mcpServer->setServerCapabilities(true, false, false, false);

    // Build set_text input schema
    QJsonObject setTextSchema;
    setTextSchema[QStringLiteral("type")] = QStringLiteral("object");
    QJsonObject textProp;
    textProp[QStringLiteral("type")] = QStringLiteral("string");
    textProp[QStringLiteral("description")] = QStringLiteral("The text to display on the label");
    QJsonObject properties;
    properties[QStringLiteral("text")] = textProp;
    setTextSchema[QStringLiteral("properties")] = properties;
    QJsonArray required;
    required.append(QStringLiteral("text"));
    setTextSchema[QStringLiteral("required")] = required;

    // Register set_text tool
    m_mcpServer->registerTool(
        QStringLiteral("set_text"),
        QStringLiteral("Set the label text displayed on the main window"),
        setTextSchema,
        std::bind(&MainWindow::handleSetText, this, std::placeholders::_1)
    );

    // Build get_text input schema (empty)
    QJsonObject getTextSchema;
    getTextSchema[QStringLiteral("type")] = QStringLiteral("object");

    // Register get_text tool
    m_mcpServer->registerTool(
        QStringLiteral("get_text"),
        QStringLiteral("Get the current text displayed on the main window label"),
        getTextSchema,
        std::bind(&MainWindow::handleGetText, this, std::placeholders::_1)
    );

    // Build delayed_set_text input schema
    QJsonObject delayedSetTextSchema;
    delayedSetTextSchema[QStringLiteral("type")] = QStringLiteral("object");
    QJsonObject delayTextProp;
    delayTextProp[QStringLiteral("type")] = QStringLiteral("string");
    delayTextProp[QStringLiteral("description")] = QStringLiteral("The text to display after a delay");
    QJsonObject delayMsProp;
    delayMsProp[QStringLiteral("type")] = QStringLiteral("number");
    delayMsProp[QStringLiteral("description")] = QStringLiteral("Delay in milliseconds (default: 1000)");
    QJsonObject asyncProperties;
    asyncProperties[QStringLiteral("text")] = delayTextProp;
    asyncProperties[QStringLiteral("delayMs")] = delayMsProp;
    delayedSetTextSchema[QStringLiteral("properties")] = asyncProperties;
    QJsonArray asyncRequired;
    asyncRequired.append(QStringLiteral("text"));
    delayedSetTextSchema[QStringLiteral("required")] = asyncRequired;

    // Register delayed_set_text ASYNC tool
    m_mcpServer->registerAsyncTool(
        QStringLiteral("delayed_set_text"),
        QStringLiteral("Set label text after a configurable delay (demonstrates async tool execution)"),
        delayedSetTextSchema,
        std::bind(&MainWindow::handleDelayedSetText, this,
                  std::placeholders::_1, std::placeholders::_2)
    );

    // Connect signals
    connect(m_mcpServer.get(), &MCPServer::toolCalled,
            this, &MainWindow::onMCPToolCalled);
    connect(m_mcpServer.get(), &MCPServer::errorOccurred,
            this, [this](const QString &err) {
        statusBar()->showMessage(QStringLiteral("MCP Error: ") + err, 5000);
    });

    m_mcpServer->start();
    m_serverRunning = true;

    m_toggleBtn->setText(QStringLiteral("Stop MCP Server"));
    m_statusLabel->setText(QStringLiteral("MCP Server: Running (%1)").arg(modeStr));
    statusBar()->showMessage(QStringLiteral("MCP Server started on %1").arg(modeStr), 3000);
}

void MainWindow::onMCPToolCalled(const QString &name, const QJsonObject &params)
{
    Q_UNUSED(params)
    qDebug() << "Tool called:" << name;
    statusBar()->showMessage(QStringLiteral("Tool '") + name + QStringLiteral("' called"), 3000);
}

QJsonObject MainWindow::handleSetText(const QJsonObject &params)
{
    QString text = params.value(QStringLiteral("text")).toString();
    if (text.isEmpty()) {
        QJsonObject result;
        result[QStringLiteral("isError")] = true;
        result[QStringLiteral("message")] = QStringLiteral("'text' parameter is required and cannot be empty");
        return result;
    }

    // Update the label on the UI thread
    ui->label->setText(text);
    statusBar()->showMessage(QStringLiteral("Label updated to: ") + text, 3000);

    QJsonObject result;
    result[QStringLiteral("success")] = true;
    result[QStringLiteral("message")] = QStringLiteral("Label text updated successfully");
    return result;
}

QJsonObject MainWindow::handleGetText(const QJsonObject &params)
{
    Q_UNUSED(params)
    QString currentText = ui->label->text();

    QJsonObject result;
    result[QStringLiteral("text")] = currentText;
    return result;
}

// ============================================================
// Async tool: delayed_set_text
// Uses QTimer::singleShot to simulate a long-running operation.
// The server does NOT block — other messages can be processed
// while the timer is ticking.
// ============================================================

void MainWindow::handleDelayedSetText(const QJsonObject &params,
                                       std::function<void(QJsonObject)> done)
{
    QString text = params.value(QStringLiteral("text")).toString();
    if (text.isEmpty()) {
        QJsonObject result;
        result[QStringLiteral("isError")] = true;
        result[QStringLiteral("message")] = QStringLiteral("'text' parameter is required and cannot be empty");
        done(result);
        return;
    }

    int delayMs = params.value(QStringLiteral("delayMs")).toInt(1000);
    if (delayMs < 0) delayMs = 0;
    if (delayMs > 30000) delayMs = 30000; // cap at 30s

    statusBar()->showMessage(
        QStringLiteral("Async tool 'delayed_set_text' scheduled in %1 ms...").arg(delayMs), 3000);

    // Use QTimer::singleShot — non-blocking, returns control to event loop
    QTimer::singleShot(delayMs, this, [this, text, done, delayMs]() {
        ui->label->setText(text);
        statusBar()->showMessage(
            QStringLiteral("Async tool 'delayed_set_text' completed after %1 ms").arg(delayMs), 5000);

        QJsonObject result;
        result[QStringLiteral("success")] = true;
        result[QStringLiteral("message")] =
            QStringLiteral("Label text updated to '%1' after %2 ms delay").arg(text).arg(delayMs);
        result[QStringLiteral("delayMs")] = delayMs;
        done(result);
    });
}
