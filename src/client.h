#ifndef CLIENT_H
#define CLIENT_H

#include <QObject>
#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QGraphicsView>
#include <QDialog>
#include <thread>
#include <mutex>

#include "network.h"
#include "player.h"

extern std::mutex consoleMutex;

class Client : public QObject
{
    Q_OBJECT
public:
    Client();
    ~Client();

    void connect(int port, const char* ip);
    void disconnect();
    void requestPlayerList();

    // void requestStartGame();
    // void requestStopGame();
    // void requestLeaveGame();
    // void requestKickPlayer();

signals:
    void connectedSuccessfully();
    void disconnectedFromServer();
    void playerListUpdated(const QStringList& players);

private:
    void createSocket();
    void receiveLoop();
    void sendMessage(const char* msg);
    bool receiveMessage(std::string& message);

    SOCKET clientSocket;
    std::thread receivingThread;
    std::atomic<bool> isRunning{false};
    std::string receiveBuffer;
};

#endif // CLIENT_H
