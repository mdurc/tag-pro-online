This program can be built and ran on both Windows and Linux.
The build system is CMake, which must be installed to build the project.

Developement and testing done on:
- macOS 14.4.1 (Apple M1 Silicon)
- Windows 11 VM on M1 mac (QEMU 7.2 Arm)
- Ubuntu 22.04 VM on M1 mac (QEMU 7.2 Arm)

Qt version 6 is the GUI tool used.
- Install qt6, qt6-base-dev, qtcreator
- Mingw is necessary to build on windows

Run build scripts from the project root directory.
- Linux: `./scripts/build-linux.sh`
- Windows: `.\scripts\build-windows.bat`
- Binary files are generated as `bin/linux/TagPro` and `bin/windows/TagPro.exe`

Arguments:
- To setup a server-only instance of the application, run the program with the flag `--server [PORT]`
- Running the program with no arguments will allow for the player to host their own server.


Dependencies:
```
sudo apt install -y \
    build-essential \
    cmake \
    qt6-base-dev \
    qt6-tools-dev \
    qt6-tools-dev-tools \
    libqt6core6 \
    libqt6gui6 \
    libqt6widgets6 \
    libqt6network6
```
