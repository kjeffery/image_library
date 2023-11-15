cmake_minimum_required(VERSION 3.17)
project(ImageLibrary)

set(CMAKE_CXX_STANDARD 23)

if (MSVC)
    add_compile_options("/Zc:preprocessor")
endif()

include_directories(Logger)

add_executable(ImageLibrary main.cpp
        propagate_const.h
        IgnoreLineCommentsBuf.h
)