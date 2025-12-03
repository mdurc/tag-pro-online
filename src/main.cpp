#include <QApplication>
#include <QMainWindow>

#include "network/server.h"
#include "gui/start_screen.h"

std::mutex consoleMutex;

int main(int argc, char* argv[]) {
  QApplication app(argc, argv);

  QMainWindow window;
  window.setWindowTitle("TagPro - Capture the Flag");
  window.resize(800, 600);
  window.setMinimumSize(500, 400);

  window.setCentralWidget(new StartScreen());
  window.show();

  return app.exec();
}
