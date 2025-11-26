#ifndef START_SCREEN_H
#define START_SCREEN_H

#include <QStackedWidget>
#include <QWidget>

class StartScreen : public QWidget {
  Q_OBJECT

public:
  StartScreen(QWidget* parent = nullptr);

private slots:
  void onHostClicked();
  void onJoinClicked();
  void onBackClicked();
  void onStartGameClicked();
  void onConnectClicked();

private:
  QStackedWidget* stackedWidget;

  void setupUI();
  QWidget* createMainMenu();
  QWidget* createHostScreen();
  QWidget* createJoinScreen();
};

#endif
