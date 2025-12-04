cmake -B build -G "MinGW Makefiles"
cmake --build build

if not exist bin\windows (
    mkdir bin\windows
)

copy build\TagPro.exe bin\windows\
