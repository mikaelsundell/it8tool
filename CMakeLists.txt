# Copyright 2023-present Contributors to the it8tool project.
# SPDX-License-Identifier: BSD-3-Clause
# https://github.com/mikaelsundell/it8tool

cmake_minimum_required (VERSION 3.23)
set (project_name "it8tool")

project ("${project_name} project")

# modules
set (CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/modules")

# imath
find_package (Imath CONFIG REQUIRED)
find_package (OpenImageIO CONFIG REQUIRED)
find_package (Eigen REQUIRED)
find_package (Rawtoaces REQUIRED)

# font
configure_file ( 
    "${PROJECT_SOURCE_DIR}/fonts/Roboto.ttf" 
    "${CMAKE_BINARY_DIR}/Roboto.ttf"
    COPYONLY
)

# package
add_executable (${project_name} "it8tool.cpp")
set_property (TARGET ${project_name} PROPERTY CXX_STANDARD 17)

target_link_libraries (${project_name}
    PRIVATE
        Imath::Imath
        OpenImageIO::OpenImageIO
        ${RAWTOACES_LIBRARIES}
)

include_directories (
    ${EIGEN_INCLUDE_DIRS}
    ${RAWTOACES_INCLUDE_DIR}
)

add_custom_command (
    TARGET ${project_name}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/fonts
        $<TARGET_FILE_DIR:${project_name}>/fonts
)

install (TARGETS ${project_name}
    RUNTIME DESTINATION bin
)

install (DIRECTORY ${CMAKE_SOURCE_DIR}/fonts
    DESTINATION bin
)
