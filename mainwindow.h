#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMessageBox>
#include <QHostAddress>
#include <QHostInfo>
#include <QTcpServer>
#include <QTcpSocket>
#include <QByteArray>
#include <QBuffer>
#include <QAudioInput>
#include <QAudioOutput>
#include <QTimer>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

private slots:
    void onReadInput(void);
    void onBtnSendClicked(void);
    void onBtnExitClicked(void);
    void onTCPNewConnection(void);
    void onTCPReadyRead(void);
    void onTCPDisConnect(void);
    void onSocketTimerTimeOut(void);
    void onCliSocketConnect(void);
    void onCliSocketReadyRead(void);
    void onCliSocketDisconnect(void);
    void onTxtDSTAddrEditingFinished(void);

private:
    void initializeAudio(void);
    void createAudioInput(void);
    void createAudioOutput(void);
    void fillAudioInputs(void);
    void fillAudioOutputs(void);

    QAudioDeviceInfo m_Inputdevice;
    QAudioDeviceInfo m_Outputdevice;
    QAudioFormat m_format;
    QAudioInput *m_audioInput;
    QAudioOutput *m_audioOutput;
    QIODevice *m_input;
    QIODevice *m_output;
    QByteArray m_buffer;

    QTcpSocket *cliSocket;

    QTimer *socketTimer;

    QTcpServer          *server;
    QList<QTcpSocket *> sockets;

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
