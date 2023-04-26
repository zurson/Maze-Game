cmake_minimum_required(VERSION 3.20)
project(Client C)

set(CMAKE_CXX_STANDARD 99)

find_package(Curses REQUIRED)
include_directories(${CURSES_INCLUDE_DIR})

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(Client client.c)

target_link_libraries(Client  ${CURSES_LIBRARIES})
target_link_libraries(Client  ${CMAKE_THREAD_LIBS_INIT})
