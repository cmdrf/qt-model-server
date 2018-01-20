cmake_minimum_required(VERSION 3.0)
project(websocket-model-server)

find_package(Qt5Core 5.5 REQUIRED)
find_package(Qt5WebSockets 5.5 REQUIRED)

set(CMAKE_AUTOMOC On)

add_library(websocket-model-server STATIC
	JsonViewModel.cpp
	JsonViewModel.h
	WebSocketModelServer.cpp
	WebSocketModelServer.h
)

target_link_libraries(websocket-model-server PUBLIC Qt5::Core Qt5::WebSockets)
target_include_directories(websocket-model-server PUBLIC .)
