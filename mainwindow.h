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
#include <QTimer>
#include <QAudioSource>
#include <QAudioFormat>
#include <QIODevice>
#include <QtMultimedia>
#include <QFile>

#define BufferSize 1024
#define PortNumber 1255

typedef struct
{
    uint32_t Sender;
    uint32_t Recipient;
    uint8_t Data[BufferSize];
}AudioPacket;


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
    void onUDPReadyRead(void);
    void onCmbAudioInputsChanged(QString str);
    void onCmbAudioOutputsChanged(QString str);

private:
    void initializeAudio(void);
    void createAudioInput(void);
    void createAudioOutput(void);
    void fillAudioInputs(void);
    void fillAudioOutputs(void);

    QAudioFormat m_format,m_formatSample;
    QAudioDevice m_Inputdevice;
    QAudioDevice m_Outputdevice;

    QAudioSource *m_audioInput;
    QAudioSink *m_audioOutput;

    QIODevice *m_input;
    QIODevice *m_output;

    QUdpSocket *Server = nullptr;
    QUdpSocket *Client = nullptr;
    void acquireMulticastLock(void);

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void showMessageBox(QString data);

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
