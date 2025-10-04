find_package(Protobuf QUIET REQUIRED)

# New comprehensive test framework
add_library(test_framework STATIC
    test_harness.cpp
    test_suite.hpp
)

target_include_directories(test_framework PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(test_framework PRIVATE -Wall -Wextra -std=c++17)
endif()

# Ensure generated protobuf headers exist before compiling test_framework
if(TARGET generate_msg_protos)
    add_dependencies(test_framework generate_msg_protos)
endif()

# Provide protobuf headers/libs when compiling files that include generated .pb.h
if(TARGET protobuf::libprotobuf)
    target_link_libraries(test_framework PUBLIC protobuf::libprotobuf)
else()
    target_include_directories(test_framework PUBLIC ${Protobuf_INCLUDE_DIRS})
    target_link_libraries(test_framework PUBLIC ${Protobuf_LIBRARIES})
endif()

# Link with core libraries
target_link_libraries(test_framework PUBLIC 
    ${CMAKE_SOURCE_DIR}/src/core/multicast_sender.cpp
    ${CMAKE_SOURCE_DIR}/src/core/multicast_receiver.cpp
)

# Main test runner executable
add_executable(test_runner
    test_runner.cpp
    test_cases.cpp
)

target_link_libraries(test_runner PRIVATE test_framework)
target_include_directories(test_runner PRIVATE ${CMAKE_SOURCE_DIR}/src)

if(TARGET msg_protos)
    target_link_libraries(test_runner PRIVATE msg_protos)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(test_runner PRIVATE -Wall -Wextra -std=c++17)
endif()
