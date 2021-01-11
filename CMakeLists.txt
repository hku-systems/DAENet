cmake_minimum_required(VERSION 3.0)
project(AnonymousP2P)

set(CMAKE_CXX_STANDARD 14)
set(THREADS_PREFER_PTHREAD_FLAG ON)

file(GLOB SRC src/*cpp src/packet/*c src/packet/*h src/packet/*pp src/app/*pp src/message/*pp src/utils/*pp src/relay/*pp src/p2p/*pp)

file(GLOB TCP_SRC src/interface/*pp src/interface/tcp/*pp)
file(GLOB TOR_SRC src/interface/*pp src/interface/tor/*pp)
file(GLOB LOOPIX_SRC src/interface/*pp src/interface/loopix/*pp)
file(GLOB DAE_SRC src/interface/*pp src/interface/dae/*pp
                  src/p2p/*pp src/packet/*pp
                  src/relay/*pp src/utils/*pp
                  src/obfuscation/ShufflePool.cpp src/obfuscation/FuzzingPool.cpp src/obfuscation/DirectPool.cpp)

add_definitions(-DNDEBUG)

find_library(GLOG glog)
find_library(SODIUM sodium)
find_package(Threads REQUIRED)

include_directories("src")

#add_executable(PingpongClient app/pingpong/PingPongClient.cpp src/interface/c_network.h ${TCP_SRC})
#add_executable(PingpongServer app/pingpong/PingPongServer.cpp app/pingpong/PingPongServer.hpp src/interface/c_network.h ${TCP_SRC})

add_executable(AnonymousP2P ${DAE_SRC} src/main.cpp src/interface/c_network.h src/interface/dae/DAESocket.cpp)

#add_executable(testOrderVector test/test_ordervector.cpp src/utils/OrderedVector.cpp)
#add_executable(testHeap test/test_heap.cpp src/utils/Heap.cpp test/test_pingpong_server.cpp)
#add_executable(testSimulator test/test_simulator.cpp src/simulation/PoolSimulator.cpp src/simulation/PoolUser.cpp)
add_executable(testpingpongserver test/test_pingpong_server.cpp ${DAE_SRC})
add_executable(testpingpongclient test/test_pingpong_client.cpp ${DAE_SRC})
#add_executable(testDAESimulator test/test_DAE_simulator.cpp src/simulation/DAESimulator.cpp)
#add_executable(testDAEProtocolSimulator test/test_DAE_protocol_simulator.cpp src/p2p/DAEService.hpp src/p2p/DAENode.cpp)
#add_executable(testMD5 test/test_md5.cpp)

add_executable(test_deaddrop_server test/test_deaddrop_server.cpp ${DAE_SRC})
add_executable(test_deaddrop_client test/test_deaddrop_client.cpp ${DAE_SRC})

add_executable(test_libevent test/test_libevent.cpp src/utils/EventTask.cpp)

add_executable(test_crypto test/testCryptoSpeed.cpp)


target_link_libraries(AnonymousP2P ${GLOG} ${SODIUM} pthread event)
target_link_libraries(testpingpongserver ${GLOG} ${SODIUM} pthread event)
target_link_libraries(testpingpongclient ${GLOG} ${SODIUM} pthread event)
target_link_libraries(test_deaddrop_server ${GLOG} ${SODIUM} pthread event)
target_link_libraries(test_deaddrop_client ${GLOG} ${SODIUM} pthread event)
target_link_libraries(test_crypto ${GLOG} ${SODIUM} pthread)
target_link_libraries(test_libevent event pthread)
#target_link_libraries(testSimulator ${SODIUM})
#target_link_libraries(PingpongClient ${GLOG} ${SODIUM} pthread)
#target_link_libraries(PingpongServer ${GLOG} ${SODIUM} pthread)
#target_link_libraries(testDAESimulator ${SODIUM})
#target_link_libraries(testDAEProtocolSimulator ${GLOG} pthread)

add_library(tcp SHARED ${TCP_SRC})
set_target_properties(tcp PROPERTIES PUBLIC_HEADER "src/interface/interface.hpp")
install(TARGETS tcp
        LIBRARY DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/lib
        PUBLIC_HEADER DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/include)
add_library(tor SHARED ${TOR_SRC} src/interface/tor/TorSocket.hpp)
set_target_properties(tor PROPERTIES PUBLIC_HEADER "src/Sharp.h")
install(TARGETS tor
        LIBRARY DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/lib
        PUBLIC_HEADER DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(loopix SHARED ${LOOPIX_SRC})
install(TARGETS loopix
        LIBRARY DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/lib)

add_library(dae SHARED ${DAE_SRC})
install(TARGETS dae
        LIBRARY DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/lib)
#include(app/CMakeLists.txt)