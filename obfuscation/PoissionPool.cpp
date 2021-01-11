//
// Created by Maxxie Jiang on 5/3/2019.
//

#include "PoissionPool.hpp"
#include <thread>
#include <cmath>
#include <sys/time.h>
#include "relay/RandomSocket.hpp"
#include "message/message.hpp"

void PoissionPool::start() {
    // dummy generation thread
    std::thread([&](){
        while (true) {
            std::this_thread::sleep_for(std::chrono::microseconds(msg_rate));
            generate_dummy();
        }
    }).detach();

    // sender thread
    // wait for the time in the first packet, or wake it up if there are newer message and the time is smaller
    std::thread([&]() {
        while (true) {
            auto packet = sync_fetch();
            if (packet != nullptr) {
                // socket->send(std::move(packet));
            }
            else {
                auto empty_packet = std::make_unique<AnonymousPacket>();
                empty_packet->header()->size = 0;
                empty_packet->header()->id = message::dummy_message;
                empty_packet->header()->relay_dest_id = 0;
                // socket->send(std::move(empty_packet));
            }
        }
    }).detach();
}

void PoissionPool::generate_dummy() {
    int t = randombytes_random() % 1000;
    int delay = t * lambda;
//    int delay = std::exp(lambda * t);
    struct timeval now;

//    LOG(INFO) << "generate packet with delay " << delay << " us";

    gettimeofday(&now, 0);
    now.tv_usec += delay;
    if (now.tv_usec > 1000000) {
        now.tv_usec -= 1000000;
        now.tv_sec += 1;
    }
    PoolPacket packet = { .delay = now, .is_dummy = true, .packet = nullptr};
    {
        std::unique_lock<std::mutex> lk(pool_mutex);
        if (pool_queue.size() >= 1024) { return; }
        pool_queue.insert(packet);
    }
    pool_cv.notify_all();
}

void PoissionPool::send(std::unique_ptr<AnonymousPacket> packet) {
    int index;
    while (true) {
        {
            index = randombytes_random();
            {
                std::unique_lock<std::mutex> lk(pool_mutex);
                int s = pool_queue.size();
                if (s == 0) continue;
                index = index % s;
                if (pool_queue[index].is_dummy) {
                    pool_queue[index].is_dummy = false;
                    pool_queue[index].packet = std::move(packet);
                    break;
                }
                // what if no message is available
                // it has to wait for the message in the latest round
            }
        }
    }
    DLOG(INFO) << "poission send success";
}

std::unique_ptr<AnonymousPacket> PoissionPool::sync_fetch() {

    struct timeval now;
    long long delay = -1;
    while (delay < 0) {
        gettimeofday(&now, 0);
        {
            std::unique_lock<std::mutex> lk(pool_mutex);
            delay = compute_delay(now) - compute_delay(pool_queue.front().delay);
            if (pool_queue.size() == 0) {
                pool_cv.wait(lk);
            } else if (delay < 0) {
                pool_cv.wait_for(lk, std::chrono::microseconds(-delay));
            }
        }
    }

    return pool_queue.pop_heap().packet;
}

bool operator<(const PoolPacket& a, const PoolPacket& b) {
    return compute_delay(a.delay) < compute_delay(b.delay);
}