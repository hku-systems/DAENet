//
// Created by jianyu on 4/15/19.
//
#include <simulation/DAEProtocolSimulator.hpp>
#include <p2p/DAEService.hpp>
int main() {

    auto simulator = new DAEProtocolSimulator(3);

    simulator->create(0);
    simulator->add_node(1);
    simulator->add_node(3);
    simulator->add_node(5);

    while(1){
        sleep(1);
        //updated finger table
        LOG(INFO) << "state-of-art finger table " ;
        simulator->nodes[0].print_info();
        simulator->nodes[1].print_info();
        simulator->nodes[3].print_info();
        simulator->nodes[5].print_info();
    }



}