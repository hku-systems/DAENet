//
// Created by tianxiang on 5/14/19.
//

#ifndef ANONYMOUSP2P_DAESERVICE_HPP
#define ANONYMOUSP2P_DAESERVICE_HPP

#include <Sharp.h>
#include <iostream>
#include <map>
#include <memory>
#include "utils/SharpTunnel.h"
#include "relay/RelayConfig.hpp"
#include <p2p/DAEProtocol.hpp>
#include <p2p/DAEService.hpp>
#include "relay/RandomSocket.hpp"
#include "obfuscation/PoissionPool.hpp"
#include "message/message.hpp"
#include "relay/SocketHeader.hpp"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#include <algorithm>
#include <app/AnonymousP2p.hpp>
#include "utils/EventTask.hpp"


class DAEService: public DAEProtocol {
public:

    //hard code joiner ip
    PeerPair joiner_node;
    int m;
    ring_id_t node_id;

    const char* node_ip;
    int node_port;

    DAENode* dae_node;

    RandomSocket *socket;

    EventTask* event_rt;

    ring_id_t get_dae_node_id(RelayConfig *config) {
        if (config->debug)
            return config->relay_id;

        int random_relay_id = randombytes_random() % config->max_relay_id;
        return random_relay_id;
    }

    void send(std::unique_ptr<AnonymousPacket> pkg, const char* ip, int dest_port) {
        pkg->header()->size = 0;
        socket->DAE_direct_send(std::move(pkg), ip, dest_port);
    }

    DAEService(RelayConfig* config)
    {
        m = config->ring_bits;
        node_id = get_dae_node_id(config);
        node_ip = config->relay_ip.c_str();
        node_port = config->relay_port;

        //TODO: should randomly find a joiner, but here we simplify this process
        joiner_node = config->init_peers;

        event_rt = new EventTask();
        dae_node = new DAENode(m, node_id, node_port, node_ip, config->bt_time, config, this);
        socket = new RandomSocket(node_ip, node_port, node_id, dae_node, *config);
    }

    //normal join
    void start(){
        EventTask::global = event_rt;

        socket->init_DAEService();

        bool check = false;
        dae_node->join(joiner_node, check);

        DLOG(INFO) << "Back to DAEService.start()" ;

        std::thread([this](){
            event_rt->start();
        }).detach();        

    }

/*    Packet format: */
//    AnonymousPacket(uint64_t _id,
//    const char* s_ip, uint64_t s_id,
//    const char* r_ip, uint64_t r_id, int r_port,
//    const char* reply_ip, uint64_t reply_id, int reply_port,
//    const char* k_ip, uint64_t k_id, int k_port,
//    int _index, bool _is_internal_msg) {
//
//        header()->id = _id;
//        header()->relay_id = s_id;
//        strncpy(header()->relay_ip, s_ip, strlen(s_ip) + 1);
//        header()->relay_dest_id = r_id;
//        strncpy(header()->relay_dest_ip, r_ip, strlen(r_ip) + 1);
//        header()->relay_dest_port = r_port;
//        header()->relay_reply_id = reply_id;
//        strncpy(header()->relay_reply_ip, reply_ip, strlen(reply_ip) + 1);
//        header()->relay_reply_port = reply_port;
//        header()->key_id = k_id;
//        strncpy(header()->key_ip, k_ip, strlen(k_ip) + 1);
//        header()->key_port = k_port;
//        header()->index = _index;
//        header()->size = 0;
//        header()->msg_type = _is_internal_msg;
//
//    }

    void join_find_successor(const char* _closest_preceding_node_addr, ring_id_t finder, int finder_port,
            const char* src_ip, ring_id_t source_id, int source_port, int index, ring_id_t dest_id, bool &is_find){

        //DLOG(INFO) << "send a join_find_successor packet";
        //DLOG(INFO) << "node "<< source_id << " asks node " << _closest_preceding_node_addr << " to look for the successor of key: " << dest_id ;
        //nodes[closest_preceding_node].handle_find_successor(source_id, index, dest_id, is_find);
        auto packet = new AnonymousPacket(message::internal_join_find_successor,
                                          src_ip, source_id,
                                          _closest_preceding_node_addr, finder, finder_port,
                                          src_ip, source_id, source_port,
                                          src_ip, dest_id, source_port,
                                          index, true);

        send(static_cast<std::unique_ptr<AnonymousPacket>>(std::move(packet)), _closest_preceding_node_addr, finder_port);
    }

    void reply_find_successor(const char* _addr, int src_port, ring_id_t dest_id, int index, ring_id_t successor, const char* key_ip, int key_port, bool &is_find){

        //DLOG(INFO) << "send a reply_find_successor packet";
        //DLOG(INFO) << "node "<< this->node_id << "replies to node " << _addr
         //         << "that the successor of key " << dest_id << "is" << successor;
        //DLOG(INFO) << _addr;
        //DLOG(INFO) << key_ip;
        //nodes[dest_id].updatesuccessor(index, successor);
        auto packet = new AnonymousPacket(message::internal_reply_find_successor,
                                          this->node_ip, this->node_id,
                                          _addr, dest_id, src_port,
                                          _addr, -1, key_port,
                                          key_ip, successor, key_port,
                                          index, true);
       // DLOG(INFO) << _addr;
        send(static_cast<std::unique_ptr<AnonymousPacket>>(std::move(packet)), _addr, src_port);

    }

    void find_predecessor(const char* _addr, int su_port, ring_id_t source, const char* src_addr, int src_port, ring_id_t id){

        //DLOG(INFO) << "send a find_predecessor packet";
       // DLOG(INFO) << "node" << source << "wants to find the predecessor of its current successor " << id;

        //nodes[id].handle_get_predecessor(source);
        auto packet = new AnonymousPacket(message::internal_find_predecessor,
                                          src_addr, source,
                                          _addr, id, su_port,
                                          src_addr, source, src_port,
                                          _addr, -1, src_port,
                                          -1, true);

        send(static_cast<std::unique_ptr<AnonymousPacket>>(std::move(packet)), _addr, su_port);
    }

    void reply_get_predecessor(const char* src_addr, int src_port, ring_id_t predecessor, const char* predecessor_ip, int predecessor_port) {

        //DLOG(INFO) << "send a reply_get_predecessor packet";
        //DLOG(INFO) << "node " << node_id << " replies that its predecessor is " << predecessor;
        //nodes[source].set_predecessor_of_succethis->node_portssor(predecessor);
        auto packet = new AnonymousPacket(message::internal_reply_get_predecessor,
                                          this->node_ip, this->node_id,
                                          src_addr, -1, src_port,
                                          src_addr, -1, src_port,
                                          predecessor_ip, predecessor, predecessor_port,
                                          -1, true);

        send(static_cast<std::unique_ptr<AnonymousPacket>>(std::move(packet)), src_addr, src_port);
    }

    void notify_predecessor_join(const char* _addr, ring_id_t successor, int succesor_port, ring_id_t id){

        //DLOG(INFO) << "send a notify_predecessor_join packet";
        //DLOG(INFO) << "node " << node_id << "notifies node " << successor << "that his predecessor could be node" << id;
        //nodes[successor].notify(id);
        auto packet = new AnonymousPacket(message::internal_notify_predecessor_join,
                                          this->node_ip, this->node_id,
                                          _addr, successor, succesor_port,
                                          _addr, -1, succesor_port,
                                          _addr, id, this->node_port,
                                          -1, true);

        send(static_cast<std::unique_ptr<AnonymousPacket>>(std::move(packet)), _addr, succesor_port);
    }

    //this->protocol->fetch_backup_successor(backup_num, this->finger_table[0].ip, this->finger_table[0].port, this->Node_ip, this->port);
    void fetch_backup_successor(int total_num, const char* su_addr, int su_port, ring_id_t src_id, const char* src_addr, int src_port){

        //DLOG(INFO) << "node " << src_id << "is feteching successor list, currently remaining " << total_num << "successors to fetch";

        auto packet = new AnonymousPacket(message::internal_fetch_successor_list,
                                          this->node_ip, this->node_id,
                                          su_addr, -1, su_port,
                                          src_addr, src_id, src_port,
                                          src_addr, src_id, src_port,
                                          total_num, true);

        send(static_cast<std::unique_ptr<AnonymousPacket>>(std::move(packet)), su_addr, su_port);
    }

    void reply_backup_successor(int counter, const char* src_addr, int src_port, ring_id_t su_id, const char* su_ip, int su_port) {

        //DLOG(INFO) << "node " << this->node_id << "is replying feteching successor list to " << src_addr
        //          << ", adding member: " << su_id << ", "<< su_ip << ", " << su_port;

        auto packet = new AnonymousPacket(message::internal_reply_successor_list,
                                          this->node_ip, this->node_id,
                                          src_addr, -1, src_port,
                                          src_addr, -1, src_port,
                                          su_ip, su_id, su_port,
                                          counter, true);

        send(static_cast<std::unique_ptr<AnonymousPacket>>(std::move(packet)), src_addr, src_port);
    }

    void check_predecessor_existence(const char* pre_addr, int pre_port, const char* src_addr, int src_port){

        //DLOG(INFO) << "node " << this->node_id << "is checking its predecessor's existence ";

        auto packet = new AnonymousPacket(message::internal_check_predecessor_existence,
                                          this->node_ip, this->node_id,
                                          pre_addr, -1, pre_port,
                                          src_addr, -1, src_port,
                                          src_addr, -1, src_port,
                                          -1, true);

        send(static_cast<std::unique_ptr<AnonymousPacket>>(std::move(packet)), pre_addr, pre_port);
    }

    void reply_predecessor_existence(ring_id_t my_id, const char* src_addr, int src_port){

        //DLOG(INFO) << "node " << this->node_id << "is replying its existence to successor";

        auto packet = new AnonymousPacket(message::internal_reply_predecessor_existence,
                                          this->node_ip, this->node_id,
                                          src_addr, -1, src_port,
                                          src_addr, -1, src_port,
                                          src_addr, my_id, src_port,
                                          -1, true);

        send(static_cast<std::unique_ptr<AnonymousPacket>>(std::move(packet)), src_addr, src_port);
    }

    void check_successor_existence(const char* su_addr, int su_port, const char* src_addr, int src_port){

        //DLOG(INFO) << "node " << this->node_id << "is checking its successor's existence ";

        auto packet = new AnonymousPacket(message::internal_check_successor_existence,
                                          this->node_ip, this->node_id,
                                          su_addr, -1, su_port,
                                          src_addr, -1, src_port,
                                          src_addr, -1, src_port,
                                          -1, true);

        send(static_cast<std::unique_ptr<AnonymousPacket>>(std::move(packet)), su_addr, su_port);
    }

    void reply_successor_existence(ring_id_t my_id, const char* src_addr, int src_port){

        //DLOG(INFO) << "node " << this->node_id << "is replying its existence to predecessor";

        auto packet = new AnonymousPacket(message::internal_reply_successor_existence,
                                          this->node_ip, this->node_id,
                                          src_addr, -1, src_port,
                                          src_addr, -1, src_port,
                                          src_addr, my_id, src_port,
                                          -1, true);

        send(static_cast<std::unique_ptr<AnonymousPacket>>(std::move(packet)), src_addr, src_port);
    }

    void external_messaging(bool is_dest, int session, int seq, ring_id_t next_id, const char* next_ip, int next_port, uint64_t type, ring_id_t dest_id, ring_id_t client_key, const char* ct_package, int size){

        DLOG(INFO) << "node " << this->node_id << "is passing an external msg to next hop: " << next_id << "content is: " << ct_package;

        auto packet = new AnonymousPacket(type,
                                          this->node_ip, this->node_id,
                                          next_ip, next_id, next_port,
                                          next_ip, client_key, next_port,
                                          next_ip, dest_id, next_port,
                                          seq, false,
                                          ct_package, size,
                                          is_dest, session);

        socket->safe_send(static_cast<std::unique_ptr<AnonymousPacket>>(std::move(packet)), next_ip, next_port);
    }

};
#endif //ANONYMOUSP2P_DAESERVICE_HPP
