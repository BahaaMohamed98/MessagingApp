cmake_minimum_required(VERSION 3.28)
project(MessagingApp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)

include_directories(include/Terminal++/src)

add_executable(messagingApp src/messagingApp.cpp)
add_executable(server src/Server.cpp)
