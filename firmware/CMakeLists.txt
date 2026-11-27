cmake_minimum_required(VERSION 3.22)

# ESP-IDF discovers these drivers without requiring a components/ directory.
set(EXTRA_COMPONENT_DIRS "${CMAKE_CURRENT_LIST_DIR}/drivers")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(gips16_checkout)
