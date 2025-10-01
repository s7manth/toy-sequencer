cmake_minimum_required(VERSION 3.10.0)
project(toy-sequencer VERSION 0.1.0 LANGUAGES C CXX)

include(CTest)
enable_testing()

add_subdirectory(src) 
add_subdirectory(tests) 

