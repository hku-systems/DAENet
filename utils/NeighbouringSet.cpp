//
// Created by jianyu on 2/21/19.
//

#include "NeighbouringSet.hpp"
#include "Sharp.h"
#include <deque>

void NeighbouringSet::add_neighbor(host_port_t &host) {
    if (set.count(host.relay_id) > 0) {
        return;
    }
    insert(host);
    set.insert(host.relay_id);
    LOG(INFO) << "add neighbor " << host.host << ":" << host.port << " " << host.relay_id;
}

void NeighbouringSet::add_neighbor(uint32_t ip, uint16_t port, uint64_t relay_id) {
    host_port_t host_port {.host = "...", .port = port, .relay_id = relay_id};
    host_port.sock.sin_port = htons(port);
    host_port.sock.sin_family = AF_INET;
    host_port.sock.sin_addr.s_addr = ip;
    if (set.count(relay_id) > 0) {
        return;
    }
    insert(host_port);
    set.insert(relay_id);
    LOG(INFO) << "add neighbor " << ip << ":" << port << " " << relay_id;
}

host_port_t& NeighbouringSet::random_neighbor() {
    int num = randombytes_random() % n;
    return arr[num];
}

// neighbour finding algo
// 1. if relay_id is in range, goto 2, else to 3
// 2. search in the range for relay_id, if not found, return its nearest neighbour (k neighbours or itself)
// 3. find the largest neighbour in the list, return it

host_port_t& NeighbouringSet::random_near_neighbor(uint64_t relay_id) {
    int size = 20;
    host_port_t h{.host = "", .port = 100, .relay_id = relay_id };
    host_port_t* arr = near_neighbour(h, size);
    int num = randombytes_random() % size;
    return arr[num];
}