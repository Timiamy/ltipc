cmake_minimum_required(VERSION 3.10)

project(ltipc VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_compile_options(-Werror -Wall -Wextra -O2)

include_directories(../ltlog/include)
include_directories(include)
include_directories(test)

add_subdirectory(src)

add_subdirectory(test)
