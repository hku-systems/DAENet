//
// Created by Maxxie Jiang on 5/3/2019.
//

#ifndef ANONYMOUSP2P_POISSIONPOOL_HPP
#define ANONYMOUSP2P_POISSIONPOOL_HPP

// main class for defending against traffic analysis
// a pool of message, real or dummy
// when a send is invoke, the message replace a random dummy message in the pool (if there is enough message)
// each pool generate (dummy) message in a small time interval
// if a message reach its deadline, the message is gone

// threads
// 1. dummy generation thread: each time interval, generate one to the message pool with exp delay
// 2. deadline sender: for some time, wake up the sender and check if it should send the message or wait
// 3. send operation, find one dummy message, if not, then generate one and attach exp delay, add it to the queue,
//    wake 2 up if the delay is less than the current min

#include <queue>
#include "utils/Heap.hpp"
#include "FuzzingPool.hpp"
#include <mutex>
#include <condition_variable>


typedef struct {
    struct timeval delay;
    bool is_dummy;
    std::unique_ptr<AnonymousPacket> packet;
} PoolPacket;

inline long long compute_delay(struct timeval a) {
    return a.tv_sec * 1000000 + a.tv_usec;
}

bool operator<(const PoolPacket& a, const PoolPacket& b);

class PoissionPool: public FuzzingPool {
private:
    // lambda of exp
    int lambda;
    AnonymousPacket shared_dummy_packet;

    // block if the head does not reach the deadline
    std::unique_ptr<AnonymousPacket> sync_fetch();

    void sync_put(PoolPacket& packet);

public:

    PoissionPool(RandomSocket *_socket): FuzzingPool(_socket, 10000) {
        lambda = 10;
    }

    Heap<PoolPacket> pool_queue = Heap<PoolPacket>(1024);
    void start();
    std::mutex pool_mutex;
    std::condition_variable pool_cv;

    void send(std::unique_ptr<AnonymousPacket> packet);
    void generate_dummy();
};


#endif //ANONYMOUSP2P_POISSIONPOOL_HPP
