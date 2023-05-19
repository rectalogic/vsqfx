cmake_minimum_required(VERSION 3.18)
cmake_policy(VERSION 3.18)

project(vsqfx-offscreen)

# Find Python
#XXX need Development? check Python3_Development_FOUND
find_package (Python3 COMPONENTS Interpreter Development.Module)
if(NOT Python3_Interpreter_FOUND)
    message(FATAL_ERROR "Python3 not found")
endif()
message(STATUS "Using python: ${Python3_EXECUTABLE}")

# Find Qt
find_package(Qt6 COMPONENTS Core Gui Quick REQUIRED)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(POLICY CMP0068)
    cmake_policy(SET CMP0068 NEW)
endif()
if(POLICY CMP0071)
  cmake_policy(SET CMP0071 NEW)
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

set(bindings_library "offscreen")
set(wrapped_header ${CMAKE_SOURCE_DIR}/bindings.h)
set(typesystem_file ${CMAKE_SOURCE_DIR}/bindings.xml)
set(generated_sources
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/offscreen_module_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/offscreenrendercontrol_wrapper.cpp
    )

# ================================== Shiboken detection ==================================
# Macro to get various pyside / python include / link flags and paths.
# Uses the not entirely supported utils/pyside_config.py file from https://code.qt.io/cgit/pyside/pyside-setup.git/tree/examples/utils/pyside_config.py
macro(pyside_config option output_var)
    if(${ARGC} GREATER 2)
        set(is_list ${ARGV2})
    else()
        set(is_list "")
    endif()

    execute_process(
      COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/pyside_config.py"
              ${option}
      OUTPUT_VARIABLE ${output_var}
      OUTPUT_STRIP_TRAILING_WHITESPACE)

    if ("${${output_var}}" STREQUAL "")
        message(FATAL_ERROR "Error: Calling ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/pyside_config.py ${option} returned no output.")
    endif()
    if(is_list)
        string (REPLACE " " ";" ${output_var} "${${output_var}}")
    endif()
endmacro()

# Query for the shiboken generator path, Python path, include paths and linker flags.
pyside_config(--shiboken-module-path shiboken_module_path)
pyside_config(--shiboken-generator-path shiboken_generator_path)
pyside_config(--pyside-path pyside_path)
pyside_config(--pyside-include-path pyside_include_dir 1)
pyside_config(--python-include-path python_include_dir)
pyside_config(--shiboken-generator-include-path shiboken_include_dir 1)
pyside_config(--shiboken-module-shared-libraries-cmake shiboken_shared_libraries 0)
pyside_config(--python-link-flags-cmake python_linking_data 0)
pyside_config(--pyside-shared-libraries-cmake pyside_shared_libraries 0)

set(shiboken_path "${shiboken_generator_path}/shiboken6${CMAKE_EXECUTABLE_SUFFIX}")
if(NOT EXISTS ${shiboken_path})
    message(FATAL_ERROR "Shiboken executable not found at path: ${shiboken_path}")
endif()

#XXX can we get_property on target for Qt headers? https://stackoverflow.com/questions/58759647/how-to-get-include-directories-from-a-target-for-use-in-add-custom-target
#XXX also see Python3_add_library and qt_add_library - can we combine?
#XXX https://cmake.org/cmake/help/latest/module/FindPython3.html?highlight=python3_add_library#commands

# Check if Qt is a framework build on macOS. This affects how include paths should be handled.
get_target_property(QtCore_is_framework Qt6::Core FRAMEWORK)
set(QT_FRAMEWORK_INCLUDES "")
if (QtCore_is_framework)
    # Get the path to the Qt framework dir.
    set(QT_FRAMEWORK_INCLUDES "--framework-include-paths=${QT6_INSTALL_PREFIX}/${QT6_INSTALL_LIBS}")
endif()

# https://discourse.cmake.org/t/how-to-find-all-the-include-folders-for-a-target-for-a-static-analyzer/3233/4
add_custom_target(
    shiboken
    COMMAND ${shiboken_path}
        --generator-set=shiboken --enable-parent-ctor-heuristic
        --enable-pyside-extensions --enable-return-value-heuristic --use-isnull-as-nb_nonzero
        --avoid-protected-hack
        ${QT_FRAMEWORK_INCLUDES}
        "-I$<JOIN:$<TARGET_PROPERTY:offscreen,INCLUDE_DIRECTORIES>,;-I>"
        "-I$<JOIN:${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES},;-I>"
        -I${CMAKE_CURRENT_SOURCE_DIR}
        -T${CMAKE_CURRENT_SOURCE_DIR}
        -T${pyside_path}/typesystems
        --output-directory=${CMAKE_CURRENT_BINARY_DIR}
        ${wrapped_header} ${typesystem_file}
    COMMAND_EXPAND_LISTS
    BYPRODUCTS ${generated_sources}
    DEPENDS ${wrapped_header} ${typesystem_file}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running generator for ${typesystem_file}."
)

# We need to include the headers for the module bindings that we use
set(pyside_additional_includes
    ${pyside_include_dir}/QtCore
    ${pyside_include_dir}/QtGui
    ${pyside_include_dir}/QtQuick
    ${pyside_include_dir}/QtNetwork
    ${pyside_include_dir}/QtOpenGL
    ${pyside_include_dir}/QtQml
)

set(SOURCES
    offscreen_render_control.cpp
    ${generated_sources}
)
Python3_add_library(offscreen MODULE ${SOURCES})
target_include_directories(offscreen PRIVATE ${shiboken_include_dir})
target_include_directories(offscreen PRIVATE ${pyside_include_dir})
target_include_directories(offscreen PRIVATE ${pyside_additional_includes})
target_link_libraries(offscreen PUBLIC Qt6::Core Qt6::Gui Qt6::GuiPrivate Qt6::Quick Qt6::QuickPrivate)
qt_finalize_target(offscreen)

add_dependencies(offscreen shiboken)

# XXX don't want to install, need to bundle in wheel and reference pyside Qt libs
# install(TARGETS offscreen LIBRARY)
