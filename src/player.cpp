#include "player.h"

Player::Player() :
    m_position(0, 0),
    m_velocity(0, 0),
    m_acceleration(0, 0) {}

void Player::update(unsigned int dt) {
    if (m_keysPressed.contains(Qt::Key_Up)) {
        m_acceleration.setY(-1);
    } else {
        m_acceleration.setY(0);
    }
    if (m_keysPressed.contains(Qt::Key_Down)) {
        m_acceleration.setY(1);
    } else {
        m_acceleration.setY(0);
    }
    if (m_keysPressed.contains(Qt::Key_Left)) {
        m_acceleration.setX(-1);
    } else {
        m_acceleration.setX(0);
    }
    if (m_keysPressed.contains(Qt::Key_Right)) {
        m_acceleration.setX(1);
    } else {
        m_acceleration.setX(0);
    }
    m_acceleration.normalize();

    m_velocity += m_acceleration * dt;
    m_velocity *= pow(m_damping, dt);
    m_position += m_velocity;
}

void Player::keyPressed(int keyCode) {
    m_keysPressed.insert(keyCode);
}

void Player::keyReleased(int keyCode) {
    m_keysPressed.remove(keyCode);
}
