add_library(simdjson_local STATIC
    vendor/simdjson.cpp
)
target_include_directories(simdjson_local PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/vendor)

add_subdirectory(msg)

# Standalone scrappy binary to subscribe to multicast events and write them
add_executable(scrappy
    applications/scrappy/scrappy.cpp
    applications/scrappy/scrappy_main.cpp
    core/multicast_receiver.cpp
)

target_include_directories(scrappy PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if(TARGET msg_protos)
    target_link_libraries(scrappy PRIVATE msg_protos msg_protos_includes)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(scrappy PRIVATE -Wall -Wextra -std=c++17)
endif()

if(WIN32)
    target_link_libraries(scrappy ws2_32)
else()
    # unix/linux - no additional libraries needed for basic socket operations
endif()

# Standalone sequencer binary
add_executable(sequencer
    applications/sequencer/sequencer_main.cpp
    core/command_sender.hpp
    core/multicast_sender.cpp
    core/multicast_receiver.cpp
)
target_include_directories(sequencer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
if(TARGET msg_protos)
    target_link_libraries(sequencer PRIVATE msg_protos msg_protos_includes)
endif()
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(sequencer PRIVATE -Wall -Wextra -std=c++17)
endif()
if(WIN32)
    target_link_libraries(sequencer ws2_32)
endif()

# Standalone ping binary
add_executable(ping
    applications/ping/ping.cpp
    applications/ping/ping_main.cpp
    core/command_sender.hpp
    core/multicast_sender.cpp
    core/multicast_receiver.cpp
)
target_include_directories(ping PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
if(TARGET msg_protos)
    target_link_libraries(ping PRIVATE msg_protos msg_protos_includes)
endif()
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(ping PRIVATE -Wall -Wextra -std=c++17)
endif()
if(WIN32)
    target_link_libraries(ping ws2_32)
endif()

# Standalone pong binary
add_executable(pong
    applications/pong/pong.cpp
    applications/pong/pong_main.cpp
    core/command_sender.hpp
    core/multicast_sender.cpp
    core/multicast_receiver.cpp
)
target_include_directories(pong PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
if(TARGET msg_protos)
    target_link_libraries(pong PRIVATE msg_protos msg_protos_includes)
endif()
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(pong PRIVATE -Wall -Wextra -std=c++17)
endif()
if(WIN32)
    target_link_libraries(pong ws2_32)
endif()

# Standalone market data binary
add_executable(market_data_app
    applications/md/market_data_main.cpp
    applications/md/http_market_data_source.hpp
    applications/md/market_data_feed.hpp
    applications/md/md_utils.hpp
    applications/md/md_notifier.hpp
    applications/md/imarket_data_source.hpp
    core/command_sender.hpp
    core/multicast_sender.cpp
)
target_include_directories(market_data_app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
if(TARGET msg_protos)
    target_link_libraries(market_data_app PRIVATE msg_protos msg_protos_includes)
endif()
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(market_data_app PRIVATE -Wall -Wextra -std=c++17)
endif()
if(WIN32)
    target_link_libraries(market_data_app ws2_32)
endif()
target_link_libraries(market_data_app PRIVATE simdjson_local)
