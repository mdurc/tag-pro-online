#include <QApplication>
#include <QMainWindow>

#include "gui/start_screen.h"
#include "network/server.h"

std::mutex consoleMutex;

int main(int argc, char* argv[]) {
  if (argc > 1 && strcmp(argv[1], "--server") == 0) {
    unsigned int port = 12345;
    if (argc > 2) port = atoi(argv[2]);

    Server server(port);
    if (!server.init()) {
      printf("Failed to start server on port %d\n", port);
      return 1;
    }

    printf("Server started on port %d\n", port);
    printf("Press Ctrl+C to stop\n");

    server.start(false);
    while (true) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
  }

  QApplication app(argc, argv);

  QMainWindow window;
  window.setWindowTitle("TagPro - Capture the Flag");
  window.setMinimumSize(500, 400);

  window.setCentralWidget(new StartScreen());
  window.adjustSize();
  window.show();

  return app.exec();
}
