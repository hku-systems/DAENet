//
// Created by jianyu on 3/10/19.
//

#include "simulation/PoolSimulator.hpp"

int main() {
    auto simulator = new PoolSimulator(100, 20);
    simulator->start();
}