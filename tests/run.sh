#!/bin/sh
# Builds and runs the core tests. No JUCE, no hardware.
set -e
cd "$(dirname "$0")"
${CXX:-g++} -std=c++17 -O1 -Wall -Wextra -Werror -o /tmp/mx400_test_core test_core.cpp
/tmp/mx400_test_core

# Studio 87 Gear Updater link (product id and link string)
(cd .. && g++ -std=c++17 -O2 -Wall -Wextra -o /tmp/s87_link_test tests/test_updater_link.cpp && /tmp/s87_link_test) || exit 1
