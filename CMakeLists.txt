cmake_minimum_required(VERSION 3.13)

project(shell-starter-cpp)

file(GLOB_RECURSE SOURCE_FILES src/*.cpp src/*.hpp)

set(CMAKE_CXX_STANDARD 23) # Enable the C++23 standard

add_executable(shell ${SOURCE_FILES})

find_package(PkgConfig QUIET)
     if(PkgConfig_FOUND)
       pkg_check_modules(Readline QUIET readline)
       if(Readline_FOUND)
         target_include_directories(shell PRIVATE ${Readline_INCLUDE_DIRS})
         target_link_libraries(shell ${Readline_LIBRARIES})
       else()
         find_path(Readline_INCLUDE_DIR readline/readline.h)
         find_library(Readline_LIBRARY NAMES readline)
         if(Readline_INCLUDE_DIR AND Readline_LIBRARY)
           target_include_directories(shell PRIVATE ${Readline_INCLUDE_DIR})
           target_link_libraries(shell ${Readline_LIBRARY})
         else()
           # Fallback to just trying to link directly
           target_link_libraries(shell readline)
         endif()
       endif()
     else()
       # Fallback to direct linking
       target_link_libraries(shell readline)
     endif()

     
