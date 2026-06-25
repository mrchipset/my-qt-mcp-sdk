#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QJsonObject>
#include <QJsonValue>
#include <memory>

class MCPClient;

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // Connection
    void onConnectClicked();
    void onDisconnectClicked();

    // Request buttons
    void onListToolsClicked();
    void onSetTextClicked();
    void onGetTextClicked();
    void onPingClicked();

    // Client signals
    void onClientConnected();
    void onClientDisconnected();
    void onClientError(const QString &errorMessage);
    void onClientInitialized();

private:
    void appendLog(const QString &message);
    void appendJson(const QString &label, const QJsonValue &val);
    void setConnectedState(bool connected);
    void showError(const QString &msg);

    Ui::MainWindow *ui;
    std::unique_ptr<MCPClient> m_client;
};

#endif // MAINWINDOW_H
