//
// Created by jianyu on 2/23/19.
//

#ifndef ANONYMOUSP2P_ORDEREDVECTOR_HPP
#define ANONYMOUSP2P_ORDEREDVECTOR_HPP

#include <set>

template<typename T>
class OrderedVector {
private:
    T *arr;
    int n;
    int max_num;
    int c_less;
    int c_large;
    T mid;
    std::set<T> set;

public:
    OrderedVector(int max, const T _mid) {
        arr = new T[max];
        n = 0;
        max_num = max;
        c_large = 0;
        c_less = 0;
        mid = _mid;
    }
    void insert(T &t) {
        if (n >= max_num) {
            if (c_large >= c_less && t > arr[max_num - 1]) return;
            if (c_less >= c_large && t < arr[0]) return;

            set.insert(t);

            bool large = false;
            // insert and sort
            if (t > mid)    c_large++;
            else            c_less++;

            if (c_large > c_less) large = true;

            bool inserted = false;
            if (large) {
                for (int i = max_num - 1;i > 0;i--) {
                    if (t > arr[i - 1] && t < arr[i]) {
                        arr[i] = t;
                        set.erase(arr[i]);
                        inserted = true;
                        break;
                    }
                    arr[i] = arr[i - 1];
                }
                if (!inserted) {
                    set.erase(arr[0]);
                    arr[0] = t;
                }
                c_large--;
            } else {
                for (int i = 0;i < max_num - 1;i++) {
                    if (t > arr[i] && t < arr[i + 1]) {
                        arr[i] = t;
                        set.erase(arr[i]);
                        inserted = true;
                        break;
                    }
                    arr[i] = arr[i + 1];
                }
                if (!inserted) {
                    set.erase(arr[max_num - 1]);
                    arr[max_num - 1] = t;
                }
                c_less--;
            }
        } else {
            // insert and sort
            bool inserted = false;
            for (int i = n;i > 0;i--) {
                if (t > arr[i - 1] && (t < arr[i] || i >= n)) {
                    arr[i] = t;
                    inserted = true;
                    break;
                }
                arr[i] = arr[i - 1];
            }
            if (!inserted) {
                arr[0] = t;
            }
            if (t > mid) c_large++;
            else if (t < mid) c_less++;
            n++;
        }
    }
    T& operator[] (int x) {
        return arr[x % n];
    }
    ~OrderedVector() {
        delete[] arr;
    }

    int size() { return n; }

    T* near_neighbour(const T &t, int &size) {
        // use the first [size] elements
        if (t <= arr[0]) {
            // fist [size]
            if (size > n) size = n;
            return arr;
        }
        if (t >= arr[n - 1]) {
            // last [size]
            if (size > n) size = n;
            return arr + n - size;
        }
        int found = 0;
        for (int i = 0;i < n - 1;i++) {
            if (t > arr[i] && t < arr[i + 1]) {
                found = i;
            }
        }
        if (found + 1 + size / 2 > n) {
            // last [size]
            if (size > n) size = n;
            return arr + n - size;
        } else if (found + 1 - size / 2 < 0) {
            // first [size]
            if (size > n) size = n;
            return arr;
        } else {
            return arr + found + 1 - size / 2;
        }
    }
};


#endif //ANONYMOUSP2P_ORDEREDVECTOR_HPP
