#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(ui->BtnSendData,&QPushButton::clicked,this,&MainWindow::onBtnSendClicked);
    connect(ui->BtnExit,&QPushButton::clicked,this,&MainWindow::onBtnExitClicked);
    connect(ui->TxtDSTAddr,&QLineEdit::editingFinished,this,&MainWindow::onTxtDSTAddrEditingFinished);


    server = new QTcpServer(this);
    server->listen(QHostAddress::Any,1010);
    connect(server,&QTcpServer::newConnection,this,&MainWindow::onTCPNewConnection);

    fillAudioInputs();
    fillAudioOutputs();

    this->statusBar()->show();

    socketTimer = new QTimer(this);
    socketTimer->setInterval(5000);
    connect(socketTimer,&QTimer::timeout,this,&MainWindow::onSocketTimerTimeOut);

    cliSocket = new QTcpSocket(this);
    connect(cliSocket,&QTcpSocket::connected,this,&MainWindow::onCliSocketConnect);
    connect(cliSocket,&QTcpSocket::readyRead,this,&MainWindow::onCliSocketReadyRead);
    connect(cliSocket,&QTcpSocket::disconnected,this,&MainWindow::onCliSocketDisconnect);

    QHostInfo info = QHostInfo::fromName(ui->TxtDSTAddr->text());
    if(!info.addresses().isEmpty())
    {
        QHostAddress address(info.addresses().at(0));
        cliSocket->connectToHost(address,1010);
    }
    socketTimer->start();
    ui->statusbar->showMessage(QString("Connecting to : %1").arg(ui->TxtDSTAddr->text()),3000);
}

MainWindow::~MainWindow()
{
    disconnect(cliSocket,&QTcpSocket::connected,this,&MainWindow::onCliSocketConnect);
    disconnect(cliSocket,&QTcpSocket::readyRead,this,&MainWindow::onCliSocketReadyRead);
    disconnect(cliSocket,&QTcpSocket::disconnected,this,&MainWindow::onCliSocketDisconnect);
    delete cliSocket;
    delete socketTimer;
    delete server;
    for(int count = 0 ; count < sockets.count() ; count++)
    {
        delete sockets.at(count);
        sockets.removeAt(count);
    }
    delete ui;
}

void MainWindow::onReadInput()
{
    if(m_input->bytesAvailable() > BufferSize)
    {
        QByteArray Buffer = m_input->read(BufferSize);
        cliSocket->write(Buffer);
    }
}

void MainWindow::onBtnExitClicked(void)
{
    QApplication::exit();
}

void MainWindow::onTCPNewConnection()
{
    sockets.append(server->nextPendingConnection());
    connect(sockets[sockets.count() - 1],&QTcpSocket::readyRead,this,&MainWindow::onTCPReadyRead);
    connect(sockets[sockets.count() - 1],&QTcpSocket::disconnected,this,&MainWindow::onTCPDisConnect);
}

void MainWindow::onTCPReadyRead()
{
    QByteArray _dataIn;
    for(int count = 0 ; count < sockets.count() ; count++)
    {
        _dataIn = sockets[count]->readAll();
        if(_dataIn.size() > 0)
        {
            if(m_output != NULL)
            {
                // qDebug() << QString("Data in : %1").arg(_dataIn.size());
                m_output->write((char *)_dataIn.data(),_dataIn.size());
            }
            else
            {
                // qDebug() << QString("Data in : %1 \r\nm_output is null").arg(_dataIn.size());
            }
        }
    }
}

void MainWindow::onTCPDisConnect()
{
    for(int count = 0 ; count < sockets.count() ; count++)
    {
        if(sockets[count]->state() != QTcpSocket::ConnectedState)
        {
            sockets.removeAt(count);
        }
    }
}

void MainWindow::onSocketTimerTimeOut()
{
    disconnect(cliSocket,&QTcpSocket::connected,this,&MainWindow::onCliSocketConnect);
    disconnect(cliSocket,&QTcpSocket::readyRead,this,&MainWindow::onCliSocketReadyRead);
    disconnect(cliSocket,&QTcpSocket::disconnected,this,&MainWindow::onCliSocketDisconnect);
    delete cliSocket;
    cliSocket = new QTcpSocket(this);
    connect(cliSocket,&QTcpSocket::connected,this,&MainWindow::onCliSocketConnect);
    connect(cliSocket,&QTcpSocket::readyRead,this,&MainWindow::onCliSocketReadyRead);
    connect(cliSocket,&QTcpSocket::disconnected,this,&MainWindow::onCliSocketDisconnect);
    QHostInfo info = QHostInfo::fromName(ui->TxtDSTAddr->text());
    if(!info.addresses().isEmpty())
    {
        QHostAddress address(info.addresses().at(0));
        cliSocket->connectToHost(address,1010);
    }
    socketTimer->start();
    ui->statusbar->showMessage(QString("Reconnecting to : %1").arg(ui->TxtDSTAddr->text()),3000);
}

void MainWindow::onCliSocketConnect()
{
    socketTimer->stop();
    ui->LblState->setText(QString("Socket Connected!"));
    ui->statusbar->showMessage(QString("Connected to : %1").arg(ui->TxtDSTAddr->text()),3000);
}

void MainWindow::onCliSocketReadyRead()
{

}

void MainWindow::onCliSocketDisconnect()
{
    ui->LblState->setText(QString("Socket Disconnected!"));
    QHostInfo info = QHostInfo::fromName(ui->TxtDSTAddr->text());
    if(!info.addresses().isEmpty())
    {
        QHostAddress address(info.addresses().at(0));
        cliSocket->connectToHost(address,1010);
    }
    socketTimer->start();
    ui->statusbar->showMessage(QString("Reconnecting to : %1").arg(ui->TxtDSTAddr->text()),3000);
}

void MainWindow::onTxtDSTAddrEditingFinished()
{
    cliSocket->close();
    ui->statusbar->showMessage(QString("Reconnecting to : %1").arg(ui->TxtDSTAddr->text()),3000);
}

void MainWindow::onBtnSendClicked()
{
    if(ui->BtnSendData->text() == QString("&Send"))
    {
        initializeAudio();
        m_input = m_audioInput->start();
        m_audioInput->setBufferSize(BufferSize);
        connect(m_input,&QIODevice::readyRead,this,&MainWindow::onReadInput);
        ui->BtnSendData->setText(QString("&Stop"));
        qDebug() << QString("Buffer Size : %1").arg(m_audioInput->bufferSize());
    }
    else
    {
        m_audioInput->stop();
        ui->BtnSendData->setText(QString("&Send"));
    }
}

void MainWindow::initializeAudio(void)
{
    m_format.setSampleRate(16000);
    m_format.setChannelCount(1); //set channels to mono
    m_format.setSampleFormat(QAudioFormat::UInt8); //set sample size to 16 bit

    qDebug() << QString("m_format : sample rate : %1 , channel count : %2 , audio Format : %3 ")
                    .arg(m_format.sampleRate()).arg(m_format.channelCount()).arg(m_format.sampleFormat());

    m_Inputdevice = QMediaDevices::audioInputs().at(ui->CmbAudioInputs->currentIndex());
    m_Outputdevice = QMediaDevices::audioOutputs().at(ui->CmbAudioOutputs->currentIndex());

    if(!m_Inputdevice.isFormatSupported(m_format))
    {
        m_format = m_Inputdevice.preferredFormat();
        qDebug() << QString("m_Inputdevice Replaced with preferredFormat(); sample rate : %1 , channel count : %2 , audio Format : %3 ")
                        .arg(m_format.sampleRate()).arg(m_format.channelCount()).arg(m_format.sampleFormat());
    }
    createAudioInput();

    if(!m_Outputdevice.isFormatSupported(m_format))
    {
        m_format = m_Inputdevice.preferredFormat();
        qDebug() << QString("m_Outputdevice Replaced with preferredFormat(); sample rate : %1 , channel count : %2 , audio Format : %3 ")
                        .arg(m_format.sampleRate()).arg(m_format.channelCount()).arg(m_format.sampleFormat());
    }
    createAudioOutput();
}

void MainWindow::createAudioInput(void)
{
    m_audioInput = new QAudioSource(m_Inputdevice, m_format, this);
}

void MainWindow::createAudioOutput(void)
{
    m_audioOutput = new QAudioSink(m_Outputdevice, m_format, this);
    m_output = m_audioOutput->start();
}

void MainWindow::fillAudioInputs()
{
    int i = 0;
    foreach (QAudioDevice info, QMediaDevices::audioInputs()) {
        ui->CmbAudioInputs->addItem(info.description(),i);
        i++;
    }
}

void MainWindow::fillAudioOutputs()
{
    int i = 0;
    foreach (QAudioDevice info, QMediaDevices::audioOutputs()) {
        ui->CmbAudioOutputs->addItem(info.description(),i);
        i++;
    }
}
