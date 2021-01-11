//
// Created by Maxxie Jiang on 28/1/2019.
//

#ifndef ANONYMOUSP2P_RANDOMSOCKET_HPP
#define ANONYMOUSP2P_RANDOMSOCKET_HPP

#include "packet/AnonymousPacket.hpp"
#include "RelayConfig.hpp"
#include "SocketHeader.hpp"
#include "utils/ConcurrentBlockQueue.hpp"

#include <queue>
#include <map>
#include <p2p/DAENode.hpp>
#include "obfuscation/FuzzingPool.hpp"

class RandomSocket {
public:
    char key[SYMMETRIC_KEY_LEN];
    int socket_fd;
    std::string host;
    uint32_t host_network;
    int port;

    int64_t total_counter = 0;
    int64_t external_counter = 0;

    host_port_t make_host(const std::string host, int port, uint64_t id);
    DAENode* in_socket_node_instance;

    uint64_t relay_id;
    void DAE_process_internal_message(uint64_t _id, std::unique_ptr<AnonymousPacket> packet);
    host_port_t init_pair;

    RelayConfig config;

    FuzzingPool *pool;
public:
    RandomSocket(std::string _host, int port, uint64_t _relay_id, DAENode* node, const RelayConfig &config);

    bool init_DAEService();
    void safe_send(std::unique_ptr<AnonymousPacket> packet, const char* ip_addr, int dest_port);

    void DAE_direct_send(std::unique_ptr<AnonymousPacket> packet, const char* ip_addr, int dest_port);
};

#endif //ANONYMOUSP2P_RANDOMSOCKET_HPP
