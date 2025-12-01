#ifndef PLAYER_H
#define PLAYER_H

#include <QVector2D>
#include <QSet>

class Player {
public:
    // input handling
    void keyPressed(int key);
    void keyReleased(int key);
    QVector2D getInputVector() const;
private:
    QSet<int> keysPressed;
};

#endif // PLAYER_H
