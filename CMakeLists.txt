cmake_minimum_required(VERSION 3.16)
project(gbemuJM)

set(CMAKE_CXX_STANDARD 14)

add_executable(gbemuJM main.cpp platform.h gb.h gb.cpp)
target_compile_features(gbemuJM PRIVATE cxx_std_14)
target_link_libraries(gbemuJM PRIVATE SDL2)