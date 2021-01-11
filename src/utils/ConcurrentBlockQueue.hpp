//
// Created by jianyu on 2/25/19.
//

#ifndef ANONYMOUSP2P_CONCURRENTBLOCKQUEUE_HPP
#define ANONYMOUSP2P_CONCURRENTBLOCKQUEUE_HPP

#include <queue>
#include <mutex>
#include <condition_variable>

template<typename T>
class ConcurrentBlockQueue {
private:
    std::queue<T> queue;
    std::mutex mutex;
    std::condition_variable cv;

public:
    void push(T t) {
        std::unique_lock<std::mutex> lk(mutex);
        queue.push(std::move(t));
        cv.notify_all();
    }

    T pop_front() {
        std::unique_lock<std::mutex> lk(mutex);
        while(queue.size() == 0) cv.wait(lk);
        T t = std::move(queue.front());
        queue.pop();
        return std::move(t);
    }

    T pop_front(int milliseconds, bool &timeout) {
        std::unique_lock<std::mutex> lk(mutex);
        while(queue.size() == 0) {
            auto status = cv.wait_for(lk, std::chrono::milliseconds(milliseconds));
            if (status == std::cv_status::timeout) {
                timeout = true;
                return T();
            }
        }
        T t = std::move(queue.front());
        queue.pop();
        return std::move(t);
    }

    int size(){
        return queue.size();
    }


};


#endif //ANONYMOUSP2P_CONCURRENTBLOCKQUEUE_HPP
