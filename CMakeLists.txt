cmake_minimum_required(VERSION 3.16)
project(gbemuJM)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0")
add_executable(gbemuJM main.cpp platform.h gb.h gb.cpp ppu.cpp ppu.h memory.cpp memory.h)
target_compile_features(gbemuJM PRIVATE cxx_std_14)
target_link_libraries(gbemuJM PRIVATE sfml-graphics sfml-window sfml-system)