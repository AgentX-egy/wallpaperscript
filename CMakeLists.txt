cmake_minimum_required(VERSION 3.13.0)

project(wallpaperscript VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

add_executable(wallpaperscript WIN32 wallpaperscript.cpp)

configure_file( ${CMAKE_SOURCE_DIR}/USAGE_MESSAGE.h.in ${CMAKE_BINARY_DIR}/USAGE_MESSAGE.h @ONLY )

target_include_directories( wallpaperscript PUBLIC
                           "${PROJECT_BINARY_DIR}"
                           )

target_compile_options( wallpaperscript PRIVATE -D_UNICODE -DUNICODE )

if(MSVC)
  target_link_options( wallpaperscript PRIVATE /SUBSYSTEM:CONSOLE )
else()
  target_link_options( wallpaperscript PRIVATE -municode )
endif()