#ifndef PLAYER_H
#define PLAYER_H

#include <QGraphicsEllipseItem>
#include <QVector2D>
#include <QDebug>
#include <QKeyEvent>

class Player : public QGraphicsEllipseItem
{
public:
    Player(bool isLocal = false);

    void keyPressEvent(QKeyEvent * event);

    void update(unsigned int dt);
private:
    float damping = 0.98;
    bool isLocal = false;
    QVector2D velocity;
    QVector2D accel;
};

#endif // PLAYER_H
