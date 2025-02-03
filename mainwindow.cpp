#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(ui->BtnSendData,&QPushButton::clicked,this,&MainWindow::onBtnSendClicked);
    connect(ui->BtnExit,&QPushButton::clicked,this,&MainWindow::onBtnExitClicked);

    fillAudioInputs();
    fillAudioOutputs();

    this->statusBar()->show();
    Server = new QUdpSocket(this);
    if(!Server->bind(PortNumber))
    {
        ui->LblState->setText(QString("Cannot Bind Server"));
    }
    else
    {
        ui->LblState->setText(QString("UDP Server Ready"));
    }
    connect(Server,&QUdpSocket::readyRead,this,&MainWindow::onUDPReadyRead);

    Client = new QUdpSocket(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onReadInput()
{
    if(ui->SpbMyID->value() == 0)
    {
        return;
    }
    if(m_input->bytesAvailable() > BufferSize)
    {
        QByteArray Buffer = m_input->read(BufferSize);
        AudioPacket packet;
        packet.Recipient = ui->SpbSendToID->value();
        packet.Sender = ui->SpbMyID->value();
        memcpy(packet.Data,Buffer.data(),BufferSize);
        QByteArray Datagram((char *)(&packet),sizeof(AudioPacket));
        Client->writeDatagram(Datagram,QHostAddress::Broadcast,PortNumber);
    }
}

void MainWindow::onBtnExitClicked(void)
{
    QApplication::exit();
}

void MainWindow::onUDPReadyRead()
{
    AudioPacket pack;
    if(!Server->hasPendingDatagrams())
        return;
    int size = Server->pendingDatagramSize();
    Server->readDatagram((char *)(&pack),size);
    if(pack.Recipient == 255 || pack.Recipient == ui->SpbMyID->value())
    {
        if(m_output != NULL)
        {
            m_output->write((char *)pack.Data,BufferSize);
        }
        else
        {
            qDebug() << QString("Data in : %1 \r\nm_output is null").arg(size);
        }
    }
    else
    {
        qDebug() << QString("pack.Recipient : %1 - pack.Sender : %2").arg(pack.Recipient).arg(pack.Sender);
    }
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
