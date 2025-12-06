#include <QApplication>
#include <QMainWindow>

#include <csignal>
#include "gui/start_screen.h"
#include "network/server.h"

std::mutex consoleMutex;

std::atomic<bool> running{true};
void signalHandler(int signum) {
    running = false;
    printf("\n[Server] Shutdown signal received (%d)\n", signum);
}

int main(int argc, char* argv[]) {
  if (argc > 1 && strcmp(argv[1], "--server") == 0) {
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

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
    while (running) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    server.stop();

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
