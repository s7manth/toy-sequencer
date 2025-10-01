add_executable(toy-sequencer 
    main.cpp
    core/multicast_sender.cpp
    sequencer.cpp
    receiver.cpp
)

target_include_directories(toy-sequencer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(toy-sequencer PRIVATE -Wall -Wextra -std=c++17)
endif()

if(WIN32)
    target_link_libraries(toy-sequencer ws2_32)
else()
    # unix/linux - no additional libraries needed for basic socket operations
endif()
