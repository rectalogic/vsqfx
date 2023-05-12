cmake_minimum_required(VERSION 3.18)
cmake_policy(VERSION 3.18)

project(offscreen VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(POLICY CMP0068)
    cmake_policy(SET CMP0068 NEW)
endif()

find_package(Qt6 COMPONENTS Core Gui Quick REQUIRED)

set(SOURCES
    offscreen_render_control.cpp
)
qt_add_library(offscreen SHARED ${SOURCES})
set_target_properties(offscreen PROPERTIES AUTOMOC ON)

target_link_libraries(offscreen PUBLIC Qt6::Core Qt6::Gui Qt6::GuiPrivate Qt6::Quick Qt6::QuickPrivate)

# XXX don't want to install, need to bundle in wheel and reference pyside Qt libs
# install(TARGETS offscreen LIBRARY)
