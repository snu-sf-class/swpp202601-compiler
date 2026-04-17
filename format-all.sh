#!/bin/bash

# Edit the variable to match your system configuration
LLVM_DIR=~/opt/llvm-22.1.0

find . -iname '*.h' -o -iname '*.cpp' | \
    xargs $LLVM_DIR/bin/clang-format -i -style=file:./.clang-format
