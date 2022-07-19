cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_AR emar)

project(webxprtcalc)
file(GLOB_RECURSE SRC_FILES ../src/*.cpp)
add_library(xprt STATIC ${SRC_FILES})
target_precompile_headers(xprt PUBLIC "../src/_header.hpp")

add_executable(webxprtcalc wasm.cpp)
target_link_options(webxprtcalc PUBLIC --bind)
target_link_options(webxprtcalc PUBLIC --sNO_DISABLE_EXCEPTION_CATCHING=1)
target_link_libraries(webxprtcalc PUBLIC xprt)
target_precompile_headers(webxprtcalc REUSE_FROM xprt)
