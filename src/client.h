#ifndef CLIENT_H
#define CLIENT_H

#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QGraphicsView>
#include <QDialog>

#include "player.h"

class Client
{
public:
    Client();

private:
    Player *player = nullptr;
};

#endif // CLIENT_H
