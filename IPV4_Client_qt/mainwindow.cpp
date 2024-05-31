#include "mainwindow.h"
#include <QtEndian>
#include <QDebug>



MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    pipe(fd);
    pid = fork();
    if (pid == 0)
    {
        ::close(fd[1]);
        dup2(fd[0], STDIN_FILENO);
        ::close(fd[0]);
        execl("/bin/sh", "sh", "-c", "/usr/bin/mpg123 - > /dev/null", NULL); 
        fprintf(stderr, "execl() : %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    ::close(fd[0]);
    worker = new Worker;
    worker->moveToThread(&workerThread);
    this->setGeometry(0, 0, 800, 480);
    udpSocket = new QUdpSocket(this);
    udpSocket->setSocketOption(QAbstractSocket::MulticastTtlOption, 1);
    pushButton[0] = new QPushButton();
    pushButton[1] = new QPushButton();
    pushButton[2] = new QPushButton();
    pushButton[3] = new QPushButton();
    hBoxLayout[0] = new QHBoxLayout();
    hBoxLayout[1] = new QHBoxLayout();
    hBoxLayout[2] = new QHBoxLayout();
    hWidget[0] = new QWidget();
    hWidget[1] = new QWidget();
    hWidget[2] = new QWidget();
    vWidget = new QWidget();
    vBoxLayout = new QVBoxLayout();
    label[0] = new QLabel();
    label[1] = new QLabel();
    label[2] = new QLabel();
    lineEdit = new QLineEdit();
    comboBox[0] = new QComboBox();
    comboBox[1] = new QComboBox();
    spinBox = new QSpinBox();
    textBrowser = new QTextBrowser();
    label[0]->setText("本地IP地址：");
    label[1]->setText("组播地址：");
    label[2]->setText("组播端口：");
    label[0]->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label[1]->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label[2]->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    spinBox->setRange(1989, 99999);
    pushButton[0]->setText("加入组播");
    pushButton[1]->setText("退出组播");
    pushButton[2]->setText("清空文本");
    pushButton[3]->setText("选择频道");
    pushButton[1]->setEnabled(false);
    lineEdit->setText("1");
    comboBox[1]->addItem("224.2.2.2");
    comboBox[1]->setEditable(true);
    hBoxLayout[0]->addWidget(pushButton[0]);
    hBoxLayout[0]->addWidget(pushButton[1]);
    hBoxLayout[0]->addWidget(pushButton[2]);
    hWidget[0]->setLayout(hBoxLayout[0]);
    hBoxLayout[1]->addWidget(label[0]);
    hBoxLayout[1]->addWidget(comboBox[0]);
    hBoxLayout[1]->addWidget(label[1]);
    hBoxLayout[1]->addWidget(comboBox[1]);
    hBoxLayout[1]->addWidget(label[2]);
    hBoxLayout[1]->addWidget(spinBox);
    hWidget[1]->setLayout(hBoxLayout[1]);
    hBoxLayout[2]->addWidget(lineEdit);
    hBoxLayout[2]->addWidget(pushButton[3]);
    hWidget[2]->setLayout(hBoxLayout[2]);
    vBoxLayout->addWidget(textBrowser);
    vBoxLayout->addWidget(hWidget[1]);
    vBoxLayout->addWidget(hWidget[0]);
    vBoxLayout->addWidget(hWidget[2]);
    vWidget->setLayout(vBoxLayout);
    setCentralWidget(vWidget);
    getLocalHostIP();
    connect(pushButton[0], SIGNAL(clicked()), this, SLOT(joinGroup()));
    connect(pushButton[1], SIGNAL(clicked()), this, SLOT(leaveGroup()));
    connect(pushButton[2], SIGNAL(clicked()), this, SLOT(clearTextBrowser()));
    connect(pushButton[3], SIGNAL(clicked()), this, SLOT(chosen()));
    connect(udpSocket, SIGNAL(readyRead()), this, SLOT(receiveMessages()));
    connect(this, SIGNAL(startWork(int,QUdpSocket *,int*)),worker, SLOT(doWork(int,QUdpSocket *,int*)));
}

MainWindow::~MainWindow()
{
    //kill(pid,SIGKILL);
}

void MainWindow::joinGroup()
{
    quint16 port = spinBox->value();
    QHostAddress groupAddr = QHostAddress(comboBox[1]->currentText());
    if (udpSocket->state() != QAbstractSocket::UnconnectedState)
        udpSocket->close();
    if (udpSocket->bind(QHostAddress::AnyIPv4, port, QUdpSocket::ShareAddress))
    {
        bool ok = udpSocket->joinMulticastGroup(groupAddr);
        textBrowser->append(ok ? "加入组播成功" : "加入组播失败");
        textBrowser->append("组播地址IP:" + comboBox[1]->currentText());
        textBrowser->append("绑定端口：" + QString::number(port));
        pushButton[0]->setEnabled(false);
        pushButton[1]->setEnabled(true);
        comboBox[1]->setEnabled(false);
        spinBox->setEnabled(false);
    }
}

void MainWindow::leaveGroup()
{
    QHostAddress groupAddr = QHostAddress(comboBox[1]->currentText());
    udpSocket->leaveMulticastGroup(groupAddr);
    udpSocket->abort();
    pushButton[0]->setEnabled(true);
    pushButton[1]->setEnabled(false);
    comboBox[1]->setEnabled(true);
    spinBox->setEnabled(true);
}

void MainWindow::getLocalHostIP()
{
    QList<QNetworkInterface> list = QNetworkInterface::allInterfaces();
    foreach (QNetworkInterface interface, list)
    {
        QList<QNetworkAddressEntry> entryList = interface.addressEntries();
        foreach (QNetworkAddressEntry entry, entryList)
        {
            if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol && !entry.ip().isLoopback())
            {
                comboBox[0]->addItem(entry.ip().toString());
                IPlist << entry.ip();
            }
        }
    }
}

void MainWindow::clearTextBrowser()
{
    textBrowser->clear();
}

void MainWindow::receiveMessages()
{
    while (udpSocket->hasPendingDatagrams())
    {
        QByteArray datagram;
        datagram.resize(udpSocket->pendingDatagramSize());
        udpSocket->readDatagram(datagram.data(), datagram.size(), &peerAddr, &peerPort);
        msg_list_t *msg_list = (msg_list_t *)datagram.data();
        if (msg_list->chnid != LISTCHNID)
            return;
        disconnect(udpSocket, SIGNAL(readyRead()), this, SLOT(receiveMessages()));
        textBrowser->append("接收来自 " + peerAddr.toString() + ":" + QString::number(peerPort));
        msg_listdesc_t *desc;
        for (desc = msg_list->list; (char *)desc < (char *)msg_list + datagram.size(); desc = (msg_listdesc_t *)((char *)desc + qFromBigEndian(desc->deslength)))
        {
            QString str = QString::fromUtf8((char *)desc->desc,qFromBigEndian(desc->deslength)-3);
            QString temp = QString("chnid = %1 description = %2").arg(desc->chnid).arg(str);
            textBrowser->append(temp);
        }
    }
}

void MainWindow::chosen()
{
    choice = lineEdit->text().toInt();
    if(!workerThread.isRunning()) {
       workerThread.start();   
    }
    worker->shutdown=true;
    sleep(1);
    worker->shutdown=false;
    emit this->startWork(choice,udpSocket,fd);
    
}

