#!/bin/sh
# Builds and runs the core tests. No JUCE, no hardware.
set -e
cd "$(dirname "$0")"
${CXX:-g++} -std=c++17 -O1 -Wall -Wextra -Werror -o /tmp/mx400_test_core test_core.cpp
/tmp/mx400_test_core
