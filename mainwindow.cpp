#include "mainwindow.h"
#include "ui_mainwindow.h"

// ------- GLOBAL POINTER -------
static MainWindow* g_mainWindowInstance = nullptr;

#ifdef Q_OS_ANDROID
extern "C"
    JNIEXPORT void JNICALL
    Java_org_verya_QWalkieTalkie_TestBridge_onMessageFromKotlin(JNIEnv* env, jclass /*clazz*/, jstring msg)
{
    if (!g_mainWindowInstance)
        return;

    QString qmsg = QJniObject(msg).toString();

    // 🧵 انتقال امن به Thread اصلی UI با Qt
    QMetaObject::invokeMethod(g_mainWindowInstance, [=]() {
        //g_mainWindowInstance->ui->TxtResult->append(QStringLiteral("From Kotlin: %1").arg(qmsg));
        g_mainWindowInstance->showMessageBox(QStringLiteral("From Kotlin: %1").arg(qmsg));
    }, Qt::QueuedConnection);
}

extern "C"
    JNIEXPORT void JNICALL
    Java_org_verya_QWalkieTalkie_TestBridge_nativeOnNotificationAction(JNIEnv *, jobject)
{
    if (!g_mainWindowInstance)
        return;

    QMetaObject::invokeMethod(g_mainWindowInstance, [=]() {
        //g_mainWindowInstance->ui->TxtResult->append(QStringLiteral("From Kotlin: %1").arg(qmsg));
        g_mainWindowInstance->showMessageBox(QStringLiteral("Action pressed"));
    }, Qt::QueuedConnection);
}
#endif

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    CurrentID = QDateTime::currentMSecsSinceEpoch();

    g_mainWindowInstance = this;
    connect(ui->BtnSendData,&QPushButton::clicked,this,&MainWindow::onBtnSendClicked);
    connect(ui->BtnExit,&QPushButton::clicked,this,&MainWindow::onBtnExitClicked);

    fillAudioInputs();
    fillAudioOutputs();

    connect(ui->CmbAudioInputs,&QComboBox::currentTextChanged,this,&MainWindow::onCmbAudioInputsChanged);
    connect(ui->CmbAudioOutputs,&QComboBox::currentTextChanged,this,&MainWindow::onCmbAudioOutputsChanged);

    initializeAudio();

    this->statusBar()->show();
#ifdef Q_OS_ANDROID
    ui->LblMyID->setText(QString("android"));
    const char* cls = "org/verya/QWalkieTalkie/TestBridge";
    QJniObject message = QJniObject::callStaticObjectMethod(
        cls,
        "notifyCPlusPlus",
        "(Ljava/lang/String;)");
    acquireMulticastLock();
#endif
    Server = new QUdpSocket(this);
    if(!Server->bind(PortNumber))
    {
        ui->LblState->setText(QString("Cannot Bind Server"));
#ifdef Q_OS_ANDROID
            QJniObject context = QNativeInterface::QAndroidApplication::context();
            if (!context.isValid())
            return;
            QJniObject jTitle = QJniObject::fromString("QWalkieTalkie");
            QJniObject jMsg   = QJniObject::fromString("Error : Cannot Bind Server");

            QJniObject::callStaticMethod<void>(
                cls,
                "postNotification",
                "(Landroid/content/Context;Ljava/lang/String;Ljava/lang/String;)V",
                context.object(),
                jTitle.object<jstring>(),
                jMsg.object<jstring>()
                );
#endif
    }
    else
    {
        ui->LblState->setText(QString("UDP Server Ready"));
#ifdef Q_OS_ANDROID
        QJniObject context = QNativeInterface::QAndroidApplication::context();
        if (!context.isValid())
            return;
        QJniObject jTitle = QJniObject::fromString("QWalkieTalkie");
        QJniObject jMsg   = QJniObject::fromString("Now UDP Server ready to use");

        QJniObject::callStaticMethod<void>(
            cls,
            "postNotification",
            "(Landroid/content/Context;Ljava/lang/String;Ljava/lang/String;)V",
            context.object(),
            jTitle.object<jstring>(),
            jMsg.object<jstring>()
            );
#endif
    }
    connect(Server,&QUdpSocket::readyRead,this,&MainWindow::onUDPReadyRead);

    Client = new QUdpSocket(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::showMessageBox(QString data)
{
    QMessageBox::about(this,QString("Callback"),QString(data));
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
        packet.RecipientGP = ui->SpbSendToID->value();
        packet.SenderGP = ui->SpbMyID->value();
        packet.SenderId = CurrentID;
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
    if(pack.SenderId == CurrentID)
    {
        return;
    }
    if(pack.RecipientGP == 255 || pack.RecipientGP == ui->SpbMyID->value())
    {
        if(m_output != NULL)
        {
            QAudioFormat outFmt = m_audioOutput->format();

            // فقط اگه فرمت‌ها با هم فرق دارن تبدیل کنیم
            bool needConvert = false;
            if (m_formatSample.sampleFormat() != outFmt.sampleFormat() ||
                m_formatSample.channelCount() != outFmt.channelCount() ||
                m_formatSample.sampleRate()   != outFmt.sampleRate())
            {
                needConvert = true;
            }

            QByteArray outData;

            if (!needConvert)
            {
                // مستقیماً بنویس چون فرمت یکیه
                outData = QByteArray((char*)pack.Data, BufferSize);
            }
            else
            {
                // ---- تبدیل UInt8 -> Int16 ----
                QByteArray tmp;
                tmp.resize(BufferSize * 2);
                int16_t* dst = reinterpret_cast<int16_t*>(tmp.data());
                const uint8_t* src = reinterpret_cast<const uint8_t*>(pack.Data);
                for (int i = 0; i < BufferSize; ++i)
                    dst[i] = ((int16_t)src[i] - 128) << 8;

                // ---- SampleRate fix: 16000 -> 48000 (3x upsample) ----
                QByteArray upsampled;
                upsampled.resize(tmp.size() * 3);
                int16_t* up = reinterpret_cast<int16_t*>(upsampled.data());
                const int16_t* in = reinterpret_cast<const int16_t*>(tmp.data());
                int sampleCount = BufferSize;

                for (int i = 0; i < sampleCount; ++i)
                {
                    up[3*i]   = in[i];
                    up[3*i+1] = in[i];
                    up[3*i+2] = in[i];
                }

                // ---- تبدیل از Mono به Stereo اگر لازم بود ----
                if (outFmt.channelCount() == 2)
                {
                    QByteArray stereo;
                    stereo.resize(upsampled.size() * 2);
                    int16_t* out = reinterpret_cast<int16_t*>(stereo.data());
                    const int16_t* src16 = reinterpret_cast<const int16_t*>(upsampled.constData());
                    int count = sampleCount * 3;
                    for (int i = 0; i < count; ++i)
                    {
                        out[2*i]   = src16[i];
                        out[2*i+1] = src16[i];
                    }
                    outData = stereo;
                }
                else
                {
                    outData = upsampled;
                }
            }
            m_output->write(outData);
        }
        else
        {
            qDebug() << QString("Data in : %1 \r\nm_output is null").arg(size);
        }
    }
    else
    {
        qDebug() << QString("pack.Recipient : %1 - pack.Sender : %2").arg(pack.RecipientGP).arg(pack.SenderGP);
    }
}

void MainWindow::onCmbAudioInputsChanged(QString str)
{
    initializeAudio();
}

void MainWindow::onCmbAudioOutputsChanged(QString str)
{
    initializeAudio();
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

    m_formatSample = m_format;

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


void MainWindow::acquireMulticastLock()
{
#ifdef Q_OS_ANDROID
    QJniObject context = QNativeInterface::QAndroidApplication::context();
    if (!context.isValid())
        return;

    QJniObject wifiManager = context.callObjectMethod(
        "getSystemService",
        "(Ljava/lang/String;)Ljava/lang/Object;",
        QJniObject::fromString("wifi").object<jstring>()
        );

    // wifi lock
    QJniObject wifiLock = wifiManager.callObjectMethod(
        "createWifiLock",
        "(ILjava/lang/String;)Landroid/net/wifi/WifiManager$WifiLock;",
        3, QJniObject::fromString("MyWifiLock").object<jstring>()
        );
    wifiLock.callMethod<void>("acquire");

    // multicast lock
    QJniObject multicastLock = wifiManager.callObjectMethod(
        "createMulticastLock",
        "(Ljava/lang/String;)Landroid/net/wifi/WifiManager$MulticastLock;",
        QJniObject::fromString("MyUdpLock").object<jstring>()
        );
    multicastLock.callMethod<void>("acquire");

    // wake lock
    QJniObject pm = context.callObjectMethod(
        "getSystemService",
        "(Ljava/lang/String;)Ljava/lang/Object;",
        QJniObject::fromString("power").object<jstring>()
        );
    QJniObject wakeLock = pm.callObjectMethod(
        "newWakeLock",
        "(ILjava/lang/String;)Landroid/os/PowerManager$WakeLock;",
        1, QJniObject::fromString("MyWakeLock").object<jstring>()
        );
    wakeLock.callMethod<void>("acquire");
#endif
}
