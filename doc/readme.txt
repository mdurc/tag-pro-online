This program can be built and ran on both Windows and Linux.
The build system is CMake, which must be installed to build the project.

Developement and testing done on:
- macOS 14.4.1 (Apple M1 Silicon)
- Windows 11 VM on the M1 mac (QEMU 7.2 Arm)

Qt version 6 is the GUI tool used.
- Install qt6, qt6-base-dev, qtcreator
- Mingw is necessary to build on windows

Run build scripts from the project root directory.
- Linux: `./scripts/build-linux.sh`
- Windows: `.\scripts\build-windows.bat`
- Binary files are generated as `bin/linux/TagPro` and `bin/windows/TagPro.exe`
