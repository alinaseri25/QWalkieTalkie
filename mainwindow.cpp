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
    qint64 len = m_audioInput->bytesReady();
    m_buffer.fill(0,len);
    qint64 l = m_input->read(m_buffer.data(), len);
    cliSocket->write(m_buffer.data(),m_buffer.count());
    this->setWindowTitle(QString("len : %1 , l : %2").arg(len).arg(l));
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
        if(_dataIn.count() > 0)
        {
            if(m_output != NULL)
            {
                this->setWindowTitle(QString("Data in : %1").arg(_dataIn.count()));
                m_output->write((char *)_dataIn.data(),_dataIn.count());
            }
            else
            {
                this->setWindowTitle(QString("Data in : %1 \r\nm_output is null").arg(_dataIn.count()));
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
        connect(m_input,&QIODevice::readyRead,this,&MainWindow::onReadInput);
        ui->BtnSendData->setText(QString("&Stop"));
    }
    else
    {
        m_audioInput->stop();
        ui->BtnSendData->setText(QString("&Send"));
    }
}

void MainWindow::initializeAudio(void)
{
    m_Inputdevice = QAudioDeviceInfo::availableDevices(QAudio::AudioInput)[ui->CmbAudioInputs->currentIndex()];
    m_Outputdevice = QAudioDeviceInfo::availableDevices(QAudio::AudioOutput)[ui->CmbAudioOutputs->currentIndex()];
    //m_format.setFrequency(8000); //set frequency to 8000
    m_format.setSampleRate(16000);
    m_format.setChannelCount(1); //set channels to mono
    m_format.setSampleSize(16); //set sample sze to 16 bit
    m_format.setSampleType(QAudioFormat::UnSignedInt ); //Sample type as usigned integer sample
    m_format.setByteOrder(QAudioFormat::LittleEndian); //Byte order
    m_format.setCodec("audio/pcm"); //set codec as simple audio/pcm

    QAudioDeviceInfo infoIn(QAudioDeviceInfo::availableDevices(QAudio::AudioInput)[ui->CmbAudioInputs->currentIndex()]);
    if (!infoIn.isFormatSupported(m_format))
    {
        //Default format not supported - trying to use nearest
        m_format = infoIn.nearestFormat(m_format);
    }
    createAudioInput();

    QAudioDeviceInfo infoOut(QAudioDeviceInfo::availableDevices(QAudio::AudioOutput)[ui->CmbAudioOutputs->currentIndex()]);
    if (!infoOut.isFormatSupported(m_format))
    {
        //Default format not supported - trying to use nearest
        m_format = infoOut.nearestFormat(m_format);
    }
    createAudioOutput();
}

void MainWindow::createAudioInput(void)
{
    if (m_input != 0) {
        disconnect(m_input, 0, this, 0);
        m_input = 0;
    }

    m_audioInput = new QAudioInput(m_Inputdevice, m_format, this);
}

void MainWindow::createAudioOutput(void)
{
    m_audioOutput = new QAudioOutput(m_Outputdevice, m_format, this);
    m_output= m_audioOutput->start();
}

void MainWindow::fillAudioInputs()
{
    int i = 0;
    foreach (QAudioDeviceInfo info, QAudioDeviceInfo::availableDevices(QAudio::AudioInput)) {
        ui->CmbAudioInputs->addItem(info.deviceName(),i);
        i++;
    }
}

void MainWindow::fillAudioOutputs()
{
    int i = 0;
    foreach (QAudioDeviceInfo info, QAudioDeviceInfo::availableDevices(QAudio::AudioOutput)) {
        ui->CmbAudioOutputs->addItem(info.deviceName(),i);
        i++;
    }
}
