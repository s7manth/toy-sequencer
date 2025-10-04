cmake_minimum_required(VERSION 3.10.0)
project(toy-sequencer VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# removing this will cause a linker error on arm64
set(CMAKE_BUILD_TYPE Release)

include(CTest)
enable_testing()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(src) 
add_subdirectory(tests)

find_program(CLANG_FORMAT_EXECUTABLE NAMES clang-format)

if(CLANG_FORMAT_EXECUTABLE)
    file(GLOB_RECURSE CLANG_FORMAT_SOURCES CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/core/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/core/*.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/applications/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/applications/*.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp
    )

    add_custom_target(clangformat
        COMMAND ${CLANG_FORMAT_EXECUTABLE} -i ${CLANG_FORMAT_SOURCES}
        COMMENT "Formatting source files with clang-format..."
        VERBATIM
    )

    add_dependencies(sequencer clangformat)
    add_dependencies(scrappy clangformat)
    add_dependencies(market_data_app clangformat)

    # test targets 
    add_dependencies(test_sequencer_basic clangformat)
    add_dependencies(test_ping_pong_integration clangformat)
    add_dependencies(test_sid_tin_functionality clangformat)
    add_dependencies(test_sid_population clangformat)
    add_dependencies(test_tin_filtering clangformat)
else()
    message(WARNING "clang-format executable not found. Code formatting target will not be available.")
endif()

