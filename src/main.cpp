#include <QApplication>
#include <QMainWindow>

#include "server.h"
#include "start_screen.h"

std::mutex consoleMutex;

int server_main(int argc, char* argv[]) {
    Server server;
    server.init();
    bool inBackground = false;
    server.run(inBackground);
    return 0;
}

int main(int argc, char* argv[]) {
  // return server_main(argc, argv);
  QApplication app(argc, argv);

  QMainWindow window;
  window.setWindowTitle("TagPro - Capture the Flag");
  window.resize(500, 400);
  window.setMinimumSize(500, 400);

  window.setCentralWidget(new StartScreen());
  window.show();

  return app.exec();
}
