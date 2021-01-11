//
// Created by jianyu on 4/10/19.
//

#ifndef ANONYMOUSP2P_DAESIMULATOR_HPP
#define ANONYMOUSP2P_DAESIMULATOR_HPP

class DAEUser;

class DAESimulator {
public:
    int num_of_users;
    int step;
    DAEUser *users;
    explicit DAESimulator(int _user): num_of_users(_user) {
        step = 0;
    }
    void start();
};


#endif //ANONYMOUSP2P_DAESIMULATOR_HPP
