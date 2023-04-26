cmake_minimum_required(VERSION 3.20)
project(Projekt_1_SO2 C)

set(CMAKE_CXX_STANDARD 99)

find_package(Curses REQUIRED)
include_directories(${CURSES_INCLUDE_DIR})

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(Projekt_1_SO2 server.c defs.c defs.h client.c client_defs.c client_defs.h)

target_link_libraries(Projekt_1_SO2  ${CURSES_LIBRARIES})
target_link_libraries(Projekt_1_SO2 ${CMAKE_THREAD_LIBS_INIT})
