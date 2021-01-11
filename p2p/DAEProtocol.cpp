//
// Created by jianyu on 4/15/19.
//

#include "DAEProtocol.hpp"
#include "Sharp.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <thread>
#include <obfuscation/DirectPool.hpp>
#include <obfuscation/UniformPool.hpp>
#include "relay/RandomSocket.hpp"
#include "obfuscation/PoissionPool.hpp"
#include "message/message.hpp"
#include "relay/SocketHeader.hpp"

ring_id_t join_find_successor(ring_id_t closest_preceding_node, ring_id_t source_id, int index, ring_id_t dest_id, bool &is_find){
    //nodes[closest_preceding_node].handle_find_successor(source_id, index, dest_id, is_find);
    //DLOG(INFO) << "in join_find_successor " ;

}

ring_id_t find_predecessor(ring_id_t source, ring_id_t id){
    //nodes[id].handle_get_predecessor(source);
}

void notify_predecessor_join(ring_id_t successor, ring_id_t id){
    //nodes[successor].notify(id);
}

void reply_find_successor(ring_id_t dest_id, int index, ring_id_t successor, bool &is_find){
    //nodes[dest_id].updatesuccessor(index, successor);
}

void reply_get_predecessor(ring_id_t source, ring_id_t predecessor) {
    //nodes[source].set_predecessor_of_successor(predecessor);
}

void print_info() {
//    for (auto &node: nodes) {
//        node.second.print_info();
//    }
}