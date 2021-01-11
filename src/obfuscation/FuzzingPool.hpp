//
// Created by jianyu on 3/23/19.
//

#ifndef ANONYMOUSP2P_FUZZINGPOOL_HPP
#define ANONYMOUSP2P_FUZZINGPOOL_HPP

#include "packet/AnonymousPacket.hpp"

class RandomSocket;

class FuzzingPool {
public:
    int msg_rate;
    RandomSocket *socket;
public:
    FuzzingPool() = delete;
    FuzzingPool(RandomSocket *_sok, int rate): socket(_sok), msg_rate(rate) {}
    virtual void start() = 0;
    virtual void send(std::unique_ptr<AnonymousPacket> packet) = 0;
};


#endif //ANONYMOUSP2P_FUZZINGPOOL_HPP
