cmake_minimum_required(VERSION 3.10)
project(TPIR_Network C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -pthread")

include_directories(server/include client/include protocol)

file(GLOB SERVER_SOURCES "server/src/*.c")
add_executable(tpir_server ${SERVER_SOURCES})
target_link_libraries(tpir_server pthread)
