#ifndef CLIENT_H
#define CLIENT_H

#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QGraphicsView>
#include <QDialog>

#include "network.h"
#include "player.h"

class Client
{
public:
    Client();

    int requestLobby();
    int joinLobby();

    void connect(int port, char * ip);
    void runGame();
private:
    void createSocket();
    SOCKET clientSocket;
    Player *player = nullptr;
};

#endif // CLIENT_H
