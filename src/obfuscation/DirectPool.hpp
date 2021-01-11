//
// Created by jianyu on 3/23/19.
//

#ifndef ANONYMOUSP2P_DIRECTPOOL_HPP
#define ANONYMOUSP2P_DIRECTPOOL_HPP

#include "FuzzingPool.hpp"
#include <relay/RandomSocket.hpp>

class DirectPool: public FuzzingPool {
public:
    explicit DirectPool(RandomSocket* _sock, int rate): FuzzingPool(_sock, rate) { }
    void send(std::unique_ptr<AnonymousPacket> packet) override;
    void start() override;
};


#endif //ANONYMOUSP2P_DIRECTPOOL_HPP
