//
// Created by jianyu on 3/10/19.
//

#include "PoolSimulator.hpp"
#include <sodium.h>
#include <tuple>
#include <iostream>
#include <chrono>

void PoolSimulator::start() {
    bool running = true;
    users = new PoolUser[cnt_users];
    for (int i = 0;i < cnt_users;i++) {
        users[i].init(i, this);
    }

    auto last_time = std::chrono::system_clock::now();
    std::chrono::duration<double> d;

    while (running) {
        auto current_time = std::chrono::system_clock::now();
        auto print = false;
        d = current_time - last_time;
        if (d.count() >= 1) {
            last_time = current_time;
            print = true;
        }

        for (int i = 0;i < cnt_users;i++) {
            if (print && i % 20 == 0 && i != 0)
                std::cout << "\n";
            users[i].run();
            if (print)
                users[i].print_info();
        }
        if (print) {
            std::cout << "\n";
            std::cout << "\033c";
        }
    }
}