cmake_minimum_required(VERSION 3.0)
project(AnonymousApplication)

set(CMAKE_CXX_STANDARD 14)
set(THREADS_PREFER_PTHREAD_FLAG ON)

file(GLOB filesharing_files ${CMAKE_CURRENT_LIST_DIR}/filesharing/*pp ${CMAKE_CURRENT_LIST_DIR}/filesharing/gsl/*pp)

# add dependencies
include_directories(${CMAKE_CURRENT_LIST_DIR}/../include/)
link_directories(${CMAKE_CURRENT_LIST_DIR}/../lib/)

# find package
find_library(GLOG glog)
find_library(SODIUM sodium)
find_library(INTERFACE dae
        HINTS ${CMAKE_CURRENT_LIST_DIR}/../lib/)

# add_executable(FileServing ${filesharing_files})
# target_link_libraries(FileServing ${GLOG} ${INTERFACE})

add_executable(PingPongServer ${CMAKE_CURRENT_LIST_DIR}/pingpong/PingPongServer.cpp)
target_link_libraries(PingPongServer ${INTERFACE} ${GLOG} ${SODIUM} pthread event)

add_executable(FileServer ${CMAKE_CURRENT_LIST_DIR}/filesharing/FileService.cpp ${CMAKE_CURRENT_LIST_DIR}/filesharing/FileTask.cpp)
target_link_libraries(FileServer ${INTERFACE} ${GLOG} ${SODIUM} pthread event)

add_executable(BufferSend ${CMAKE_CURRENT_LIST_DIR}/buffersend/BufferSend.cpp)
target_link_libraries(BufferSend ${INTERFACE} ${GLOG} ${SODIUM} pthread event)