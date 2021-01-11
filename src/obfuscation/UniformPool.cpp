//
// Created by jianyu on 3/23/19.
//

#include "UniformPool.hpp"
#include <thread>
#include "message/message.hpp"
#include "relay/RandomSocket.hpp"
#include <sodium.h>

void UniformPool::start() {
    std::thread([&]() {
        while (true) {
            std::this_thread::sleep_for(std::chrono::microseconds(msg_rate));
            send_batch();
        }
    }).detach();
}

void UniformPool::send_batch() {
    for (auto &p: pool_queue) {
        if (p.second.size() == 0) {
            // TODO: send dummy
        } else {
            auto pkg = p.second.pop_heap();
            host_port_t host_port = {
                    .host = pkg.ip,
                    .port = pkg.port
            };
            socket->direct_send(std::move(pkg.packet), host_port);
        }
    }
}

void UniformPool::send(uint64_t finger_id, std::unique_ptr<AnonymousPacket> packet) {
    uint64_t rnd_id = randombytes_random() % 100;
    UniformPoolPackage pkg = {
            .random_id = rnd_id,
            .id = finger_id,
            .ip = std::string(fingers[finger_id].ip),
            .port = fingers[finger_id].port,
            .packet = std::move(packet)
    };
    pool_queue[finger_id].insert(pkg);
}