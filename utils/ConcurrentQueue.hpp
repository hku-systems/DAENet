//
// Created by Maxxie Jiang on 23/3/2019.
//

#ifndef ANONYMOUSP2P_CONCURRENTQUEUE_HPP
#define ANONYMOUSP2P_CONCURRENTQUEUE_HPP

#include <queue>
#include <mutex>

template<typename T>
class ConcurrentQueue {
private:
    std::queue<std::unique_ptr<T>> queue;
    std::mutex mutex;
public:
    void push(std::unique_ptr<T> t) {
        std::unique_lock<std::mutex> lk(mutex);
        queue.push(std::move(t));
    }

    std::unique_ptr<T> pop_front() {
        std::unique_lock<std::mutex> lk(mutex);
        if (queue.size() == 0) return nullptr;
        auto t = std::move(queue.front());
        queue.pop();
        return std::move(t);
    }
};

#endif //ANONYMOUSP2P_CONCURRENTQUEUE_HPP
