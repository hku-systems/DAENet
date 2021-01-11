//
// Created by jianyu on 3/10/19.
//

#ifndef ANONYMOUSP2P_POOLUSER_HPP
#define ANONYMOUSP2P_POOLUSER_HPP

#include <vector>
#include <tuple>
#include <queue>

class PoolSimulator;
class PoolUser {
private:
    // send pool - from, dest, cnt
    std::tuple<int, int, int> *sent_pool;
    std::queue<std::tuple<int, int, int, bool>> recv_pool;
    int cnt_send = 0;
    int index;
    PoolSimulator *pool;
public:
    void init(int _idx, PoolSimulator *_pool) {
        index = _idx;
        pool = _pool;
        sent_pool = new std::tuple<int, int, int>[1000];
    }

    PoolUser() {}
    int run();
    void send_dummy();
    void send_msg(int index);
    void recv_msg(int from, int dest, int cnt, bool drop);
    void handle_send_msg(int from, int dest, int cnt, bool drop);
    void print_info();
};


#endif //ANONYMOUSP2P_POOLUSER_HPP
