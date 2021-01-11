//
// Created by jianyu on 3/23/19.
//

#include "DirectPool.hpp"

void DirectPool::send(std::unique_ptr<AnonymousPacket> packet) {
     socket->DAE_direct_send(std::move(packet), NULL, 0);
}

void DirectPool::start() {
    std::this_thread::sleep_for(std::chrono::microseconds(msg_rate));
}