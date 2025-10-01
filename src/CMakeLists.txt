add_executable(toy-sequencer 
    main.cpp
    core/multicast_sender.cpp
    applications/sequencer.cpp
    applications/receiver.cpp
)

target_include_directories(toy-sequencer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Generated protos for typed messages
add_subdirectory(msg)

if(TARGET msg_protos)
    target_link_libraries(toy-sequencer PRIVATE msg_protos msg_protos_includes)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(toy-sequencer PRIVATE -Wall -Wextra -std=c++17)
endif()

if(WIN32)
    target_link_libraries(toy-sequencer ws2_32)
else()
    # unix/linux - no additional libraries needed for basic socket operations
endif()
