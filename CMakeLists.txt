cmake_minimum_required(VERSION 3.10)

PROJECT(hctunnel)

# set(CMAKE_VERBOSE_MAKEFILE ON)

SET(CMAKE_CXX_COMPILER "g++")

set (CMAKE_CXX_STANDARD 17)

set(CMAKE_BUILD_TYPE "Debug")

set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -Wall -g -DDEBUG") 
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")

# INCLUDE_DIRECTORIES(
# include
# )

set(SRC_FILES
    ./src/main.cpp
    ./src/eventloop.cpp
    ./src/server.cpp
    ./src/epoller.cpp
    ./src/utils.cpp
    ./src/acceptor.cpp
    ./src/postman.cpp
    ./src/httpmsg.cpp
    ./src/threadpool.cpp
)

ADD_EXECUTABLE(${PROJECT_NAME} ${SRC_FILES})

TARGET_LINK_LIBRARIES(${PROJECT_NAME} pthread)