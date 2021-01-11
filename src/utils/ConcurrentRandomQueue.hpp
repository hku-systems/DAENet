//
// Created by jianyu on 6/10/19.
//

#ifndef ANONYMOUSP2P_CONCURRENTBLOCKRANDOMQUEUE_HPP
#define ANONYMOUSP2P_CONCURRENTBLOCKRANDOMQUEUE_HPP

#include <sodium.h>
#include <cstdlib>
#include <ctime>
#include <cstdio>

template<typename T>
class ConcurrentRandomQueue {
private:
    T *arr;
    int n;
    std::mutex mutex;

public:
    ConcurrentRandomQueue(): ConcurrentRandomQueue(10240) {
        srand(time(NULL));
    }
    ConcurrentRandomQueue(int _n) {
        n = 0;
        arr = new T[_n];
        srand(time(NULL));
    }
    void push(T t) {
        std::unique_lock<std::mutex> lk(mutex);
        arr[n++] = std::move(t);
    }

    T pop_front() {
        std::unique_lock<std::mutex> lk(mutex);
        if (n == 0) return nullptr;
        T t = std::move(arr[n - 1]);
        n--;
        return std::move(t);
    }

    T pop_random(int min_pool) {
        uint32_t idx_s = randombytes_random();
        if (min_pool == 0) {
            std::unique_lock<std::mutex> lk(mutex);
            if (n == 0) return nullptr;
            int idx = idx_s % n;
            T t = std::move(arr[idx]);
            if (idx != (n - 1)) {
                arr[idx] = std::move(arr[n - 1]);
                arr[n - 1] = nullptr;
            }
            n--;
            return std::move(t);
        } else {
            std::unique_lock<std::mutex> lk(mutex);
            int pool_max = std::max(n, min_pool);
            int idx = idx_s % pool_max;
            if (idx >= n) return nullptr;
            T t = std::move(arr[idx]);
            if (idx != (n - 1))
                arr[idx] = std::move(arr[n - 1]);
            n--;
            return std::move(t);
        }
    }
    T pop_with_P(double p) {
        //if(n == 0){
        //    return nullptr;
        //}
        int r = rand();
        if(r <= p * RAND_MAX){
            return pop_random(0);
        }else{
            return nullptr;
        }
    }

    int size(){
        return n;
    }


};

#endif //ANONYMOUSP2P_CONCURRENTBLOCKRANDOMQUEUE_HPP
