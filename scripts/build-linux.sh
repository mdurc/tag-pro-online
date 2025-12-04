#!/bin/bash
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -B build && \
cmake --build build && \
mkdir -p bin/linux && \
cp build/TagPro bin/linux
