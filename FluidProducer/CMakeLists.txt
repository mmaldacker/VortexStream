cmake_minimum_required(VERSION 3.14)

project(VortexStream)

set(CMAKE_CXX_STANDARD 14)

include(FetchContent)

FetchContent_Declare(
  vortex2d
  GIT_REPOSITORY https://github.com/mmaldacker/Vortex2D.git
  GIT_TAG        v1.8.0
)
FetchContent_MakeAvailable(vortex2d)

message("Search dir ${RABBITMQC_DIR}")

find_path(Rabbitmqc_INCLUDE_DIR
  NAMES amqp.h
    HINTS ${RABBITMQC_DIR}/include
  )

find_library(Rabbitmqc_LIBRARY
  NAMES rabbitmq.4
    HINTS ${RABBITMQC_DIR}/lib
  )

add_executable(vortex_stream_compute main.cpp)
target_link_libraries(vortex_stream_compute vortex2d glm ${Rabbitmqc_LIBRARY})
target_include_directories(vortex_stream_compute PRIVATE ${Rabbitmqc_INCLUDE_DIR})
