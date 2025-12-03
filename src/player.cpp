#include "player.h"

#include <QKeyEvent>

void Player::keyPressed(int key) { keysPressed.insert(key); }
void Player::keyReleased(int key) { keysPressed.remove(key); }

QVector2D Player::getInputVector() const {
  QVector2D input(0, 0);

  if (keysPressed.contains(Qt::Key_Up)) input.setY(-1);
  if (keysPressed.contains(Qt::Key_Down)) input.setY(1);
  if (keysPressed.contains(Qt::Key_Left)) input.setX(-1);
  if (keysPressed.contains(Qt::Key_Right)) input.setX(1);

  input.normalize();

  return input;
}
