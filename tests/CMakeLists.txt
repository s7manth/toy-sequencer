find_package(Protobuf QUIET REQUIRED)

add_library(test_support STATIC
    support/test_harness.cpp
)

target_include_directories(test_support PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/support
    ${CMAKE_CURRENT_SOURCE_DIR}/stubs
)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(test_support PRIVATE -Wall -Wextra -std=c++17)
endif()

# Ensure generated protobuf headers exist before compiling test_support
if(TARGET generate_msg_protos)
    add_dependencies(test_support generate_msg_protos)
endif()

# Provide protobuf headers/libs when compiling files that include generated .pb.h
if(TARGET protobuf::libprotobuf)
    target_link_libraries(test_support PUBLIC protobuf::libprotobuf)
else()
    target_include_directories(test_support PUBLIC ${Protobuf_INCLUDE_DIRS})
    target_link_libraries(test_support PUBLIC ${Protobuf_LIBRARIES})
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

# Individual SID/TIN functionality tests
add_executable(test_instance_id_assignment
    cases/test_instance_id_assignment.cpp
    ${CMAKE_SOURCE_DIR}/src/applications/sequencer.cpp
)
target_link_libraries(test_instance_id_assignment PRIVATE test_support)
target_include_directories(test_instance_id_assignment PRIVATE ${CMAKE_SOURCE_DIR}/src)
if(TARGET msg_protos)
    target_link_libraries(test_instance_id_assignment PRIVATE msg_protos)
endif()
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(test_instance_id_assignment PRIVATE -Wall -Wextra -std=c++17)
endif()
add_test(NAME test_instance_id_assignment COMMAND test_instance_id_assignment)

add_executable(test_sid_population
    cases/test_sid_population.cpp
    ${CMAKE_SOURCE_DIR}/src/applications/sequencer.cpp
)
target_link_libraries(test_sid_population PRIVATE test_support)
target_include_directories(test_sid_population PRIVATE ${CMAKE_SOURCE_DIR}/src)
if(TARGET msg_protos)
    target_link_libraries(test_sid_population PRIVATE msg_protos)
endif()
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(test_sid_population PRIVATE -Wall -Wextra -std=c++17)
endif()
add_test(NAME test_sid_population COMMAND test_sid_population)

add_executable(test_tin_filtering
    cases/test_tin_filtering.cpp
    ${CMAKE_SOURCE_DIR}/src/applications/sequencer.cpp
)
target_link_libraries(test_tin_filtering PRIVATE test_support)
target_include_directories(test_tin_filtering PRIVATE ${CMAKE_SOURCE_DIR}/src)
if(TARGET msg_protos)
    target_link_libraries(test_tin_filtering PRIVATE msg_protos)
endif()
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(test_tin_filtering PRIVATE -Wall -Wextra -std=c++17)
endif()
add_test(NAME test_tin_filtering COMMAND test_tin_filtering)

add_executable(test_ping_pong_integration
    cases/test_ping_pong_integration.cpp
    ${CMAKE_SOURCE_DIR}/src/applications/sequencer.cpp
    ${CMAKE_SOURCE_DIR}/src/applications/ping.cpp
    ${CMAKE_SOURCE_DIR}/src/applications/pong.cpp
)
target_link_libraries(test_ping_pong_integration PRIVATE test_support)
target_include_directories(test_ping_pong_integration PRIVATE ${CMAKE_SOURCE_DIR}/src)
if(TARGET msg_protos)
    target_link_libraries(test_ping_pong_integration PRIVATE msg_protos)
endif()
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(test_ping_pong_integration PRIVATE -Wall -Wextra -std=c++17)
endif()
add_test(NAME test_ping_pong_integration COMMAND test_ping_pong_integration)

# Comprehensive SID/TIN functionality test suite
add_executable(test_sid_tin_functionality
    cases/test_sid_tin_functionality.cpp
    ${CMAKE_SOURCE_DIR}/src/applications/sequencer.cpp
    ${CMAKE_SOURCE_DIR}/src/applications/ping.cpp
    ${CMAKE_SOURCE_DIR}/src/applications/pong.cpp
)
target_link_libraries(test_sid_tin_functionality PRIVATE test_support)
target_include_directories(test_sid_tin_functionality PRIVATE ${CMAKE_SOURCE_DIR}/src)
if(TARGET msg_protos)
    target_link_libraries(test_sid_tin_functionality PRIVATE msg_protos)
endif()
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(test_sid_tin_functionality PRIVATE -Wall -Wextra -std=c++17)
endif()
add_test(NAME test_sid_tin_functionality COMMAND test_sid_tin_functionality)
