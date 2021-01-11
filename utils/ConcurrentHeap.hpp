//
// Created by jianyu on 3/6/19.
//

#ifndef ANONYMOUSP2P_CONCURRENT_HEAP_HPP
#define ANONYMOUSP2P_CONCURRENT_HEAP_HPP

#include <iostream>

template<typename T>
class ConcurrentHeap {
private:
    T* arr;
    int n;
    int max_size;
    std::mutex mutex;

public:
    ConcurrentHeap(int _max_size) {
        // get the nearest pow 2 max_size
        max_size = _max_size;
        n = 0;
        arr = new T[max_size];
    }

    ConcurrentHeap(): ConcurrentHeap(8192) {}

    void insert(T& t) {
        std::unique_lock<std::mutex> lk(mutex);
        int index = n;
        int father;
        n++;
        while (index > 0) {
            father = index >> 1;
            if (t < arr[father])
                arr[index] = std::move(arr[father]);
            else
                break;
            index >>= 1;
        }
        arr[index] = std::move(t);
    }

    T pop_heap() {
        std::unique_lock<std::mutex> lk(mutex);
        T t = std::move(arr[0]);
        if (n > 0) {
            int last_index = n - 1;
            T &last = arr[last_index];
            n--;
            int index = 0;
            int left, right;
            while (index < n) {
                left = index * 2 + 1;
                right = index * 2 + 2;
                if (left < n && (right >= n || arr[left] < arr[right]) && arr[left] < last) {
                    // insert left
                    arr[index] = std::move(arr[left]);
                    index = left;
                } else if (right < n && arr[right] < last) {
                    // insert right
                    arr[index] = std::move(arr[right]);
                    index = right;
                } else {
                    // insert current
                    break;
                }
            }
            arr[index] = std::move(last);
        }
        return t;
    }

    T& front() {
        return arr[0];
    }

    void print_all() {
        for (int i = 0;i < n;i++) {
            std::cout << arr[i] << " ";
        }
        std::cout << "\n";
    }
    int size() { return n; }

    T& operator[](int index) {
        return arr[index % n];
    }
};


#endif //ANONYMOUSP2P_CONCURRENT_HEAP_HPP
