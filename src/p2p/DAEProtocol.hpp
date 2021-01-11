//
// Created by jianyu on 4/15/19.
//

#ifndef ANONYMOUSP2P_DAEPROTOCOL_HPP
#define ANONYMOUSP2P_DAEPROTOCOL_HPP

#include <Sharp.h>
#include "DAENode.hpp"

// the chord protocol only provide interface for communicate with other node
class DAEProtocol {

public:
    //send a msg to ask closest_preceding_node find the successor of key dest_id
    virtual void join_find_successor(const char* _closest_preceding_node_addr,ring_id_t finder, int finder_port, const char* src_addr,
            ring_id_t source_id, int source_port, int index, ring_id_t dest_id, bool &is_find) = 0;
    //reply to dest_id the successor of fingertable[index]
    virtual void reply_find_successor(const char* addr, int src_port, ring_id_t key, int index, ring_id_t successor, const char* key_ip, int key_port, bool &is_find) = 0;
    //send a msg to ask id(source's successor) find its predecessor
    virtual void find_predecessor(const char* successor_addr, int successor_port, ring_id_t source, const char* src_addr, int src_port, ring_id_t id) = 0;
    //reply to source its successor's predecessor (update variable:this->predecessor_of_successor)
    virtual void reply_get_predecessor(const char* src_addr, int src_port, ring_id_t predecessor, const char* predecessor_ip, int predecessor_port) = 0;
    //send a msg to successor to remind a possible predecessor id
    virtual void notify_predecessor_join(const char* successor_addr, ring_id_t successor, int successor_port, ring_id_t id) = 0;
    //send a msg to fetch a list of backup nodes/successors
    virtual void fetch_backup_successor(int total_num, const char* su_addr, int su_port, ring_id_t src_id, const char* src_addr, int src_port) = 0;
    //reply to fetch successor list
    virtual void reply_backup_successor(int counter, const char* src_addr, int src_port, ring_id_t su_id, const char* su_ip, int su_port) = 0;
    //send to check predecessor's existance
    virtual void check_predecessor_existence(const char* pre_addr, int pre_port, const char* src_addr, int src_port) = 0;
    //reply to check predecessor's existance
    virtual void reply_predecessor_existence(ring_id_t my_id, const char* src_addr, int src_port) = 0;
    //send to check successor's existance
    virtual void check_successor_existence(const char* su_addr, int su_port, const char* src_addr, int src_port) = 0;
    //reply to check successor's existance
    virtual void reply_successor_existence(ring_id_t my_id, const char* src_addr, int src_port) = 0;
    //send an external/app message to dest_id
    virtual void external_messaging(bool is_dest, int session, int seq, ring_id_t next_id, const char* next_ip, int next_port, uint64_t type, ring_id_t dest_id, ring_id_t client_key, const char* ct_package, int size) = 0;
};

#endif //ANONYMOUSP2P_DAEPROTOCOL_HPP
