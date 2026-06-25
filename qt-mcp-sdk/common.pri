# common.pri — Qt MCP SDK shared build configuration
# All subprojects should include this file

# Unified output directory to build/bin/
DESTDIR = $$PWD/../build/bin

# Intermediate files go to build/intermediates/<module>/, avoid polluting source
BUILD_DIR = $$PWD/../build/intermediates/$$TARGET
OBJECTS_DIR = $$BUILD_DIR/obj
MOC_DIR = $$BUILD_DIR/moc
UI_DIR = $$BUILD_DIR/ui
RCC_DIR = $$BUILD_DIR/rcc

# C++ standard
CONFIG += c++17

# Warnings
CONFIG += warn_on
