# CMakeLists.txt for Wisp Engine Examples
cmake_minimum_required(VERSION 3.5)

# Set the project name
set(PROJECT_NAME "wisp-engine-demo")
project(${PROJECT_NAME})

# Set the component directories
set(EXTRA_COMPONENT_DIRS 
    ../src/system
    ../src/boards
)

# Include ESP-IDF build system
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Define the project
project(${PROJECT_NAME})
