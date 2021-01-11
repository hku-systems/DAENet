//
// Created by jianyu on 3/23/19.
//

#ifndef ANONYMOUSP2P_UNIFORMPOOL_HPP
#define ANONYMOUSP2P_UNIFORMPOOL_HPP


#include "FuzzingPool.hpp"
#include <map>
#include <queue>
#include <p2p/DAENode.hpp>
#include "utils/ConcurrentQueue.hpp"
#include "utils/Heap.hpp"

// for the uniform pool
// each msg_rate, the sender send [[ neighbour set ]] message to all its neighbors
// it then find the message in the list, if there is one, then send it out

// how to store the message in range?
// we can store it in a hashmap

typedef struct {
    uint64_t random_id;
    uint64_t id;
    std::string ip;
    int port;
    std::unique_ptr<AnonymousPacket> packet;
} UniformPoolPackage;

inline bool operator<(const UniformPoolPackage &l, const UniformPoolPackage &r) {
    return l.random_id < r.random_id;
}

// procedure for sending a message
// from the neighbor list, find it in range_list,
// or send a pool_queue_random (larger or lesser)
class UniformPool {
private:
    std::map<int, Heap<UniformPoolPackage> > pool_queue;
    void send_batch();
public:
    int msg_rate;
    RandomSocket *socket;
    ChordFinger *fingers;

    explicit UniformPool(int _rate,
            RandomSocket *_sok,
            ChordFinger* _fingers): msg_rate(_rate),
                                    socket(_sok),
                                    fingers(_fingers)  {}
    void start();
    void send(uint64_t finger_id, std::unique_ptr<AnonymousPacket> packet);
};


#endif //ANONYMOUSP2P_UNIFORMPOOL_HPP
