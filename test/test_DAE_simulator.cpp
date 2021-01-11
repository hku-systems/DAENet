//
// Created by jianyu on 4/10/19.
//

#include <simulation/DAESimulator.hpp>

int main () {
    auto simulator = new DAESimulator(1024);
    simulator->start();
}