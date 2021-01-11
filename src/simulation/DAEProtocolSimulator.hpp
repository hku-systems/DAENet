//
// Created by jianyu on 4/15/19.
//

#ifndef ANONYMOUSP2P_DAEPROTOCOLSIMULATOR_HPP
#define ANONYMOUSP2P_DAEPROTOCOLSIMULATOR_HPP


#include <p2p/DAEProtocol.hpp>
#include <map>

class DAEProtocolSimulator: public DAEProtocol {
public:

    int cnt_node = 0;
    int m;

    DAEProtocolSimulator(int _m): m(_m) {}

    std::map<ring_id_t, DAENode> nodes;

    ring_id_t first_node;

    //first node join & initialize Chord Ring
    void create(ring_id_t node_id) {

        auto node = DAENode(m, node_id, this);
        nodes[node_id] = node;

        first_node = node_id;

        nodes[node_id].create();

        cnt_node += 1;
    }

    //normal join
    void add_node(ring_id_t node_id) {
        LOG(INFO) << "in simulator";
        auto node = DAENode(m, node_id, this);
        nodes[node_id] = node;

        bool check = false;
        //should random find a joiner, but here we simplify this process
        nodes[node_id].join(first_node, check);

        cnt_node += 1;
    }

    //void add_random_node(ring_id_t node);

    ring_id_t join_find_successor(ring_id_t closest_preceding_node, ring_id_t source_id, int index, ring_id_t dest_id, bool &is_find){
        nodes[closest_preceding_node].handle_find_successor(source_id, index, dest_id, is_find);
    }

    ring_id_t find_predecessor(ring_id_t source, ring_id_t id){
        nodes[id].handle_get_predecessor(source);
    }

    void notify_predecessor_join(ring_id_t successor, ring_id_t id){
        nodes[successor].notify(id);
    }

    void reply_find_successor(ring_id_t dest_id, int index, ring_id_t successor, bool &is_find){
        nodes[dest_id].updatesuccessor(index, successor);
    }

    void reply_get_predecessor(ring_id_t source, ring_id_t predecessor) {
        nodes[source].set_predecessor_of_successor(predecessor);
    }

    void print_info() {
        for (auto &node: nodes) {
            node.second.print_info();
        }
    }


};


#endif //ANONYMOUSP2P_DAEPROTOCOLSIMULATOR_HPP
