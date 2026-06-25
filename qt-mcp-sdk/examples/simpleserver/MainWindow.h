#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QSpinBox>
#include <QLineEdit>
#include <memory>

class MCPServer;

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr, bool autoStartStdio = false);
    ~MainWindow();

    // Start the server in stdio mode immediately (used for CLI auto-start)
    void startStdioServer();

private slots:
    void onStartStopClicked();
    void onMCPToolCalled(const QString &name, const QJsonObject &params);

private:
    void setupToolsAndStart(const QString &modeStr);
    QJsonObject handleSetText(const QJsonObject &params);
    QJsonObject handleGetText(const QJsonObject &params);

    Ui::MainWindow *ui;
    QComboBox  *m_modeCombo  = nullptr;
    QLineEdit  *m_hostEdit   = nullptr;
    QSpinBox   *m_portSpin   = nullptr;
    QPushButton *m_toggleBtn = nullptr;
    QLabel     *m_statusLabel = nullptr;
    std::unique_ptr<MCPServer> m_mcpServer;
    bool m_serverRunning = false;
};
#endif // MAINWINDOW_H
