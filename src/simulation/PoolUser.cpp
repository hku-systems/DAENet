//
// Created by jianyu on 3/10/19.
//

#include "PoolUser.hpp"
#include "PoolSimulator.hpp"
#include <sodium.h>
#include <tuple>
#include "PoolSimulator.hpp"
#include <iostream>
#include <cmath>

int PoolUser::run() {
    // generate msg for the current user
    int p = randombytes_random() % 10;
    if (p > 8) {
        int dest = (this->index + 40) % pool->cnt_users;
        if (dest != this->index)
            sent_pool[cnt_send++] = std::make_tuple(index, dest, 0);
    }

    while (!recv_pool.empty()) {
        auto head = recv_pool.front();
        recv_pool.pop();
        recv_msg(std::get<0>(head), std::get<1>(head), std::get<2>(head), std::get<3>(head));
    }

    // send msg
    int k = randombytes_random() % std::max(200, cnt_send);
    if (k >= cnt_send) {
        // sent dummy msg
        send_dummy();
    } else {
        // sent sender msg
        send_msg(k);
    }
    return 0;
}

void PoolUser::send_dummy() {
    int dest = randombytes_random() % pool->cnt_users;
    if (dest == this->index) return;
    int receiver;
    int d = dest - this->index;

    int real_d = (std::abs(d) > pool->cnt_users / 2)? pool->cnt_users  - std::abs(d) : std::abs(d);
    if (real_d < pool->range / 2) {
        // within the range
//        std::cout << dest << " " << this->index << " to dest\n";
        receiver = dest;
    } else if (this->index - dest > pool->cnt_users / 2
               || (d > 0 && d < pool->cnt_users / 2)) {
        receiver = (this->index + randombytes_random() % (pool->range / 2)) % pool->cnt_users;
    } else {
        receiver = (this->index - randombytes_random() % (pool->range / 2) + pool->cnt_users) % pool->cnt_users;
    }

    pool->users[receiver].handle_send_msg(index, dest, 0, false);
}

void PoolUser::send_msg(int index) {
    int receiver;
    int from = std::get<0>(sent_pool[index]);
    int dest = std::get<1>(sent_pool[index]);
    int cnt  = std::get<2>(sent_pool[index]);

    int d = dest - this->index;

    int real_d = (std::abs(d) > pool->cnt_users / 2)? pool->cnt_users  - std::abs(d) : std::abs(d);
    if (real_d < pool->range / 2) {
        // within the range
//        std::cout << dest << " " << this->index << " to dest\n";
        receiver = dest;
    } else if (this->index - dest > pool->cnt_users / 2
    || (d > 0 && d < pool->cnt_users / 2)) {
        receiver = (this->index + randombytes_random() % (pool->range / 2)) % pool->cnt_users;
    } else {
        receiver = (this->index - randombytes_random() % (pool->range / 2) + pool->cnt_users) % pool->cnt_users;
    }

    int p = (1 - std::pow(1.1, -cnt_send / 20)) * std::exp(-5/(cnt + 1)) * 100;


    int flip = randombytes_random() % 100;
    bool drop = flip <= p;

//    std::cout << p << " " << flip << "\n";

    sent_pool[index].swap(sent_pool[cnt_send - 1]);
    cnt_send--;
    pool->users[receiver].handle_send_msg(from, dest, cnt, drop);
}

void PoolUser::recv_msg(int from, int dest, int cnt, bool drop) {
    if (!drop) {
        sent_pool[cnt_send++] = std::make_tuple(from, dest, cnt + 1);
    } else {
//        std::cout << "drop in " << cnt << "\n";
    }
}

void PoolUser::print_info() {
    std::cout << cnt_send << "\t";
}

void PoolUser::handle_send_msg(int from, int dest, int cnt, bool drop) {
    // msg is well-received
    if (dest == this->index) {
        return;
    }
    recv_pool.push(std::make_tuple(from, dest, cnt, drop));
}