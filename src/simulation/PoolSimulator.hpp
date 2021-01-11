//
// Created by jianyu on 3/10/19.
//

#ifndef ANONYMOUSP2P_POOLSIMULATOR_HPP
#define ANONYMOUSP2P_POOLSIMULATOR_HPP

#include <vector>
#include "PoolUser.hpp"

class PoolSimulator {
public:
    int cnt_users;
    int range;
    PoolUser* users;
    explicit PoolSimulator(int _cnt_user, int _range): cnt_users(_cnt_user), range(_range) {}
    void start();

};


#endif //ANONYMOUSP2P_POOLSIMULATOR_HPP
