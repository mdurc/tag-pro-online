#ifndef CLIENT_H
#define CLIENT_H

#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QGraphicsView>
#include <QDialog>
#include <thread>
#include <mutex>

#include "network.h"
#include "player.h"

extern std::mutex consoleMutex;

class Client
{
public:
    Client();
    ~Client();

    int requestLobby();
    int joinLobby();

    void connect(int port, const char * ip);

    void requestStartGame();
    void requestStopGame();
    void requestLeaveGame();
    void requestKickPlayer();

    void notifyKeyPressed();
    void notifyKeyReleased();
private:
    void createSocket();
    void sendmsg(char * msg);
    SOCKET clientSocket;

    std::thread receivingThread;
    void receiveLoop();
    bool isRunning = false;
};

#endif // CLIENT_H
