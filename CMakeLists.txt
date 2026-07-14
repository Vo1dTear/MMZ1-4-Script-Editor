cmake_minimum_required(VERSION 3.16)

project(MMZScriptEditor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt automatic tools
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find the required Qt6 modules
find_package(Qt6 6.5 REQUIRED COMPONENTS Widgets)

# Source file list
set(SOURCES
    main.cpp
    scripteditor.cpp
    scripttexteditor.cpp
)

set(HEADERS
    scripteditor.h
    scripttexteditor.h
    editcommand.h
)

# Executable
add_executable(${PROJECT_NAME} WIN32
    ${SOURCES}
    ${HEADERS}
)

# Link against Qt6 Widgets
target_link_libraries(${PROJECT_NAME}
    Qt6::Widgets
)
