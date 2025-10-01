add_library(test_support STATIC
    support/test_harness.cpp
    ${CMAKE_SOURCE_DIR}/src/applications/receiver.cpp
)

target_include_directories(test_support PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/support
    ${CMAKE_CURRENT_SOURCE_DIR}/stubs
)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(test_support PRIVATE -Wall -Wextra -std=c++17)
endif()

add_executable(test_sequencer_basic
    cases/test_sequencer_basic.cpp
    ${CMAKE_SOURCE_DIR}/src/applications/sequencer.cpp
)
target_link_libraries(test_sequencer_basic PRIVATE test_support)
target_include_directories(test_sequencer_basic PRIVATE ${CMAKE_SOURCE_DIR}/src)
if(TARGET msg_protos)
    target_link_libraries(test_sequencer_basic PRIVATE msg_protos)
endif()
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(test_sequencer_basic PRIVATE -Wall -Wextra -std=c++17)
endif()
add_test(NAME test_sequencer_basic COMMAND test_sequencer_basic)
