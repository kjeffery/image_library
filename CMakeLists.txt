cmake_minimum_required(VERSION 3.17)
project(ImageLibrary)

set(CMAKE_CXX_STANDARD 23)

if (MSVC)
    add_compile_options("/Zc:preprocessor")
endif()

add_executable(ImageLibrary main.cpp
)