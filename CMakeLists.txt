# Copyright 2022-present Contributors to the it8tool project.
# SPDX-License-Identifier: BSD-3-Clause
# https://github.com/mikaelsundell/logctool

cmake_minimum_required(VERSION 3.23)
set( project_name "it8tool" )

project( "${project_name} project" )

# modules
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/modules" )

# imath
find_package( Imath REQUIRED )
find_package( OIIO REQUIRED )
find_package( OCIO REQUIRED )
find_package( Eigen REQUIRED )

# font
configure_file( 
    "${PROJECT_SOURCE_DIR}/fonts/Roboto.ttf" 
    "${CMAKE_BINARY_DIR}/Roboto.ttf"
    COPYONLY
)

# package
add_executable( ${project_name} "it8tool.cpp" )
set_property( TARGET ${project_name} PROPERTY CXX_STANDARD 14)

include_directories (
    ${IMATH_INCLUDE_DIRS}
    ${OIIO_INCLUDE_DIRS}
    ${OCIO_INCLUDE_DIR}
)

target_link_libraries( ${project_name}
    ${IMATH_LIBRARIES}  
    ${OIIO_LIBRARIES}
    ${OCIO_LIBRARIES}
)
