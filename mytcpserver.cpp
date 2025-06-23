#include "mytcpserver.h"
#include <QTimer>

const int MyTcpServer::MAX_CLIENTS = 3;

MyTcpServer::MyTcpServer(QObject *parent)
    : QObject(parent),
    mTcpServer(new QTcpServer(this))
{
    connect(mTcpServer, &QTcpServer::newConnection, this, &MyTcpServer::slotNewConnection);

    if (!mTcpServer->listen(QHostAddress::Any, 1234)) {
        qDebug() << "Server failed to start!";
    } else {
        qDebug() << "Server started on port" << mTcpServer->serverPort();
    }
}

MyTcpServer::~MyTcpServer()
{
    for (QTcpSocket* client : mClients) {
        client->disconnectFromHost();
        client->deleteLater();
    }
    mTcpServer->close();
}

void MyTcpServer::slotNewConnection()
{
    QTcpSocket *clientSocket = mTcpServer->nextPendingConnection();

    if (!clientSocket)
        return;

    if (mClients.size() >= MAX_CLIENTS) {
        clientSocket->write("Server is full. Please try again later.\r\n");
        clientSocket->flush();
        clientSocket->disconnectFromHost();
        clientSocket->deleteLater();
        qDebug() << "Rejected connection: max clients reached.";
        return;
    }

    mClients.append(clientSocket);

    connect(clientSocket, &QTcpSocket::readyRead, this, &MyTcpServer::slotServerRead);
    connect(clientSocket, &QTcpSocket::disconnected, this, &MyTcpServer::slotClientDisconnected);

    qDebug() << "New client connected:" << clientSocket->peerAddress().toString();

    clientSocket->write("Welcome to the quiz game!\r\n");
    clientSocket->flush();

    QTimer::singleShot(100, this, &MyTcpServer::notifyClientsWaiting);
}

void MyTcpServer::slotServerRead()
{
    QTcpSocket *clientSocket = qobject_cast<QTcpSocket*>(sender());
    if (!clientSocket || !quizStarted) return;

    QByteArray data = clientSocket->readAll();
    QString answer = QString::fromUtf8(data).trimmed().toLower();

    qDebug() << "Answer from" << clientSocket->peerAddress().toString() << ":" << answer;

    processAnswer(clientSocket, answer);
}


void MyTcpServer::slotClientDisconnected()
{
    QTcpSocket *clientSocket = qobject_cast<QTcpSocket*>(sender());
    if (!clientSocket)
        return;

    qDebug() << "Client disconnected:" << clientSocket->peerAddress().toString();
    mClients.removeAll(clientSocket);
    clientSocket->deleteLater();
    QTimer::singleShot(100, this, &MyTcpServer::notifyClientsWaiting);
}

void MyTcpServer::notifyClientsWaiting()
{
    int remaining = MAX_CLIENTS - mClients.size();
    QString msg;

    if (remaining > 0) {
        msg = QString("Waiting for %1 more player(s)...\r\n").arg(remaining);
    } else {
        msg = "All players connected! Starting quiz...\r\n";
    }

    QByteArray data = msg.toUtf8();

    for (QTcpSocket* client : mClients) {
        if (client->state() == QAbstractSocket::ConnectedState) {
            client->write(data);
            client->flush();
        }
    }

    qDebug() << msg.trimmed();

    if (remaining == 0 && !quizStarted) {
        quizStarted = true;
        QTimer::singleShot(500, this, &MyTcpServer::startQuiz);
    }
}

void MyTcpServer::startQuiz()
{
    if (currentQuestionIndex < questions.size()) {
        QString question = questions[currentQuestionIndex];
        QByteArray data = question.toUtf8();

        for (QTcpSocket* client : mClients) {
            if (client->state() == QAbstractSocket::ConnectedState) {
                client->write(data);
                client->flush();
            }
        }

        qDebug() << "Sent:" << question.trimmed();

        currentQuestionIndex++;
        QTimer::singleShot(15000, this, &MyTcpServer::startQuiz);
    } else {
        finishQuiz();
    }
}


void MyTcpServer::processAnswer(QTcpSocket* client, const QString& answer)
{
    if (currentQuestionIndex == 0 || currentQuestionIndex > correctAnswers.size())
        return;

    QString correct = correctAnswers[currentQuestionIndex - 1];
    if (answer == correct) {
        scores[client] += 1;
        client->write("Correct!\r\n");
    } else {
        client->write("Incorrect.\r\n");
    }
    client->flush();
}

void MyTcpServer::finishQuiz()
{
    int maxScore = 0;
    for (int score : scores.values()) {
        if (score > maxScore)
            maxScore = score;
    }

    for (QTcpSocket* client : mClients) {
        QString msg;
        int score = scores.value(client, 0);

        if (score == maxScore && maxScore > 0) {
            msg = QString("Quiz over! You win! Score: %1\r\n").arg(score);
        } else {
            msg = QString("Quiz over! Try again. Your score: %1\r\n").arg(score);
        }

        client->write(msg.toUtf8());
        client->flush();
    }

    qDebug() << "Quiz finished. Clients notified.";
}

