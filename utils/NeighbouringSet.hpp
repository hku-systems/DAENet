//
// Created by jianyu on 2/21/19.
//

#ifndef ANONYMOUSP2P_NEIGHBOURINGSET_HPP
#define ANONYMOUSP2P_NEIGHBOURINGSET_HPP

#include <vector>
#include <set>
#include <set>
#include <cstdint>
#include "relay/SocketHeader.hpp"
#include "OrderedVector.hpp"

class NeighbouringSet {
private:
    host_port_t *arr;
    int n;
    int c_less;
    int c_large;
    host_port_t mid;
    std::set<uint64_t> set;

    static const int MAX_SET = 20;

    void insert(host_port_t &t) {
        if (n >= MAX_SET) {
            if (c_large >= c_less && t > arr[MAX_SET - 1]) return;
            if (c_less >= c_large && t < arr[0]) return;

            set.insert(t.relay_id);

            bool large = false;
            // insert and sort
            if (t > mid)    c_large++;
            else            c_less++;

            if (c_large > c_less) large = true;

            bool inserted = false;
            if (large) {
                for (int i = MAX_SET - 1;i > 0;i--) {
                    if (t > arr[i - 1] && t < arr[i]) {
                        arr[i] = t;
                        set.erase(arr[i].relay_id);
                        inserted = true;
                        break;
                    }
                    arr[i] = arr[i - 1];
                }
                if (!inserted) {
                    set.erase(arr[0].relay_id);
                    arr[0] = t;
                }
                c_large--;
            } else {
                for (int i = 0;i < MAX_SET - 1;i++) {
                    if (t > arr[i] && t < arr[i + 1]) {
                        arr[i] = t;
                        set.erase(arr[i].relay_id);
                        inserted = true;
                        break;
                    }
                    arr[i] = arr[i + 1];
                }
                if (!inserted) {
                    set.erase(arr[MAX_SET - 1].relay_id);
                    arr[MAX_SET - 1] = t;
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

    host_port_t* near_neighbour(const host_port_t &t, int &size) {
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
public:

    class iterator {
    private:
        host_port_t *arr;
        int pos;
    public:
        iterator(host_port_t* _arr, int _pos): arr(_arr), pos(_pos) {}
        host_port_t& operator*() { return arr[pos]; }
        host_port_t* operator->() { return &(operator*()); }
        iterator& operator++() { pos+= 1; return *this; }
        iterator operator++(int plus) { return iterator(arr, pos + plus); }
        bool operator==(const iterator& other) const { return arr == other.arr && pos == other.pos; }
        bool operator!=(const iterator& other) const { return !operator==(other); }
    };

    iterator begin() { return iterator(arr, 0); }
    iterator end() { return iterator(arr, size()); }

    host_port_t& operator[] (int x) {
        return arr[x % n];
    }

    host_port_t& first() { return arr[0]; }
    host_port_t& last()  { return arr[n - 1]; }

    void init(const host_port_t &relay_host) {
        arr = new host_port_t[MAX_SET];
        mid = relay_host;
        n = 0;
        c_large = 0;
        c_less = 0;
    }

    void add_neighbor(host_port_t &host);

    void add_neighbor(uint32_t ip, uint16_t port, uint64_t relay_id);

    host_port_t& random_neighbor();

    // given a relay id, find its near neighbor randomly, or return the relay if found
    host_port_t& random_near_neighbor(uint64_t relay_id);

    int size() { return n; }

    bool contain(uint64_t id) { return set.find(id) != set.end(); }

    //~NeighbouringSet() { delete[] arr; }
};


#endif //ANONYMOUSP2P_NEIGHBOURINGSET_HPP
