#include <QApplication>
#include <QMainWindow>

#include "start_screen.h"

int main(int argc, char* argv[]) {
  QApplication app(argc, argv);

  QMainWindow window;
  window.setWindowTitle("TagPro - Capture the Flag");
  window.resize(1000, 800);
  window.setMinimumSize(500, 400);

  window.setCentralWidget(new StartScreen());
  window.show();

  return app.exec();
}
