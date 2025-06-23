#ifndef MYTCPSERVER_H
#define MYTCPSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QList>
#include <QByteArray>
#include <QDebug>

class MyTcpServer : public QObject
{
    Q_OBJECT

public:
    explicit MyTcpServer(QObject *parent = nullptr);
    ~MyTcpServer();

public slots:
    void slotNewConnection();
    void slotClientDisconnected();
    void slotServerRead();

private:
    QTcpServer *mTcpServer;
    QList<QTcpSocket*> mClients;

    static const int MAX_CLIENTS;

    void notifyClientsWaiting();
    void startQuiz();
    void finishQuiz();
    void processAnswer(QTcpSocket* client, const QString& answer);

    bool quizStarted = false;
    int currentQuestionIndex = 0;
    QStringList questions = {
        "Question 1: What is the capital of France?\r\n",
        "Question 2: What is 2 + 2?\r\n",
        "Question 3: Who wrote 'Hamlet'?\r\n"
    };
    QStringList correctAnswers = {
        "paris", "4", "shakespeare"
    };

    QMap<QTcpSocket*, int> scores;
};

#endif // MYTCPSERVER_H
