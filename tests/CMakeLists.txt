add_executable(test_multicast 
    test_multicast.cpp
    ${CMAKE_SOURCE_DIR}/src/core/multicast_sender.cpp
    ${CMAKE_SOURCE_DIR}/src/applications/receiver.cpp
)

target_include_directories(test_multicast PRIVATE ${CMAKE_SOURCE_DIR}/src)


if(WIN32)
    target_link_libraries(test_multicast ws2_32)
else()
    # unix/linux - no additional libraries needed for basic socket operations
endif()

# compiler flags
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(test_multicast PRIVATE -Wall -Wextra -std=c++17)
endif()

add_test(NAME test_multicast COMMAND test_multicast)
