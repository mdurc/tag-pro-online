#include "player.h"

Player::Player(bool isLocal)
    : velocity(0, 0),
    accel(0, 0)
{
    if (isLocal) {
        this->isLocal = isLocal;
        setFlag(QGraphicsItem::ItemIsFocusable);
        setFocus();
    }

}

void Player::update(unsigned int dt) {
    velocity += accel * dt;
    velocity *= pow(damping, dt);
    setPos(x() + velocity.x(), y() + velocity.y());
}

void Player::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Left)
    {
        accel.setX(1);
    }
    else if (event->key() == Qt::Key_Right)
    {
        accel.setX(-1);
    }
    else
    {
        accel.setX(0);
    }
    if (event->key() == Qt::Key_Up)
    {
        accel.setY(-1);
    }
    else if (event->key() == Qt::Key_Down)
    {
        accel.setY(1);
    }
    else
    {
        accel.setY(0);
    }
}
