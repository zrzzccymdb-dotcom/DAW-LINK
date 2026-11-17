
cmake_minimum_required(VERSION 3.20)

project(DAW_LINK
    VERSION 0.1.0
    DESCRIPTION "Universal low-latency audio transport for DAWs and OBS"
    LANGUAGES CXX
)

if(NOT WIN32)
    message(FATAL_ERROR "DAW LINK Phase 1 currently supports Windows only.")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(dawlink_transport STATIC
    src/audio_transport.cpp
)

target_include_directories(dawlink_transport
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_definitions(dawlink_transport
    PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
)

add_executable(dawlink_test_sender
    tools/test_sender.cpp
)

target_link_libraries(dawlink_test_sender
    PRIVATE
        dawlink_transport
)

add_executable(dawlink_test_receiver
    tools/test_receiver.cpp
)

target_link_libraries(dawlink_test_receiver
    PRIVATE
        dawlink_transport
)

if(MSVC)
    target_compile_options(dawlink_transport PRIVATE /W4 /permissive-)
    target_compile_options(dawlink_test_sender PRIVATE /W4 /permissive-)
    target_compile_options(dawlink_test_receiver PRIVATE /W4 /permissive-)
else()
    target_compile_options(dawlink_transport PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(dawlink_test_sender PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(dawlink_test_receiver PRIVATE -Wall -Wextra -Wpedantic)
endif()
