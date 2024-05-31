#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QUdpSocket>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QTextBrowser>
#include <QLabel>
#include <QComboBox>
#include <QSpinBox>
#include <QHostInfo>
#include <QLineEdit>
#include <QNetworkInterface>
#include <QDebug>
#include <protocol.h>
#include <client.h>
#include <stdlib.h>
#include <stdio.h>
#include <QThread>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <QString>
#include <signal.h>
class Worker;
static ssize_t writen(int fd, const void *buf, size_t count)
{
    size_t len, total, ret;
    total = count;
    for (len = 0; total > 0; len += ret, total -= ret)
    {
    again:
        ret = write(fd, buf + len, total);
        if (ret < 0)
        {
            if (errno == EINTR) // 中断系统调用，重启 write
                goto again;
            fprintf(stderr, "write() : %s\n", strerror(errno));
            return -1;
        }
    }
    return len;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    QUdpSocket *udpSocket;
    QPushButton *pushButton[4];
    QLabel *label[3];
    QWidget *hWidget[3];
    QHBoxLayout *hBoxLayout[3];
    QWidget *vWidget;
    QVBoxLayout *vBoxLayout;
    QTextBrowser *textBrowser;
    QComboBox *comboBox[2];
    QSpinBox *spinBox;
    QLineEdit *lineEdit;
    int choice;
    QList<QHostAddress> IPlist;
    void getLocalHostIP();
    QHostAddress peerAddr;
    quint16 peerPort;
    int fd[2];
    QThread workerThread;
    Worker *worker;
    pid_t pid;
    
private slots:
    void joinGroup();
    void leaveGroup();
    void clearTextBrowser();
    void receiveMessages();
    void chosen();

signals:
    void startWork(int, QUdpSocket *, int *);
};

class Worker : public QObject
{
    Q_OBJECT
public:
    bool shutdown = false;
public slots:
    void doWork(int choice, QUdpSocket *udpSocket, int *fd)
    {
        while (1)
        {
            while (udpSocket->hasPendingDatagrams())
            {
                QByteArray datagram;
                datagram.resize(udpSocket->pendingDatagramSize());
                udpSocket->readDatagram(datagram.data(), datagram.size(), NULL, NULL);
                msg_channel_t *msg_channel = (msg_channel_t *)datagram.data();
                if (msg_channel->chnid == choice)
                {
                    writen(fd[1], msg_channel->data, datagram.size() - sizeof(msg_channel->chnid));
                }
            }
            if (shutdown)
                return;
        }
    }
};
#endif // MAINWINDOW_H
