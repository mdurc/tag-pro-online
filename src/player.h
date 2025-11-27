#ifndef PLAYER_H
#define PLAYER_H

#include <QGraphicsEllipseItem>
#include <QVector2D>
#include <QDebug>
#include <QKeyEvent>

class Player
{
public:
    Player();

    const QVector2D& getPosition() { return m_position; }
    void setPosition(QVector2D& position) { m_position = position; }
    void update(unsigned int dt);

    void keyPressed(int keyCode);
    void keyReleased(int keyCode);
private:
    float m_damping = 0.98;

    QSet<int> m_keysPressed;
    QVector2D m_position;
    QVector2D m_velocity;
    QVector2D m_acceleration;
};

#endif // PLAYER_H
