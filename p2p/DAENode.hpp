//
// Created by jianyu on 4/15/19.
//

#ifndef ANONYMOUSP2P_DAENODE_HPP
#define ANONYMOUSP2P_DAENODE_HPP

#include <Sharp.h>
#include <relay/SocketHeader.hpp>
#include <memory>
#include <cmath>
#include <list>
#include <mutex>
#include <relay/RelayConfig.hpp>
#include <packet/AnonymousPacket.hpp>
#include "utils/ConcurrentBlockQueue.hpp"
#include "utils/ConcurrentQueue.hpp"

typedef struct {

    bool is_deaddrop;

    int session_id;

    int seq_id;

    uint64_t msgtype;

    ring_id_t to_id;

    ring_id_t from_id;

    char msg[MAX_MTU];

    int size_of_payload;

}MSG_INFO;

class MsgDescriptor{
public:
    MSG_INFO info;
    MsgDescriptor(bool is_dd, int session, int seq, uint64_t msg_type, ring_id_t connector_id, ring_id_t client_key, char *buf, int size){
        info.is_deaddrop = is_dd;

        info.session_id = session;

        info.seq_id = seq;

        info.msgtype = msg_type;

        info.to_id = connector_id;

        info.from_id = client_key;

        memcpy(info.msg, buf, size);

        info.size_of_payload = size;
    }

    MsgDescriptor(){}
};

class ChordFinger {
public:
    ring_id_t start;  // start of the finger
    ring_id_t to;     // finer to
    ring_id_t successor;
    char ip[20];
    int port;

    void set_successor(ring_id_t su) { successor = su; }

    ChordFinger() = default;
};

class DAEProtocol;

class DAENode {
public:
    DAEProtocol *protocol;

    // num_of_bit
    int m;

    int bootstrap_time;

    // current node info
    ring_id_t id;

    char Node_ip[20];

    int port;

    volatile bool successor_status;
    // lock
    bool successor_onchecking;

    //predecessor info
    ring_id_t predecessor;

    char predecessor_ip[20];

    int predecessor_port;

    volatile bool predecessor_status;
    // lock
    bool predecessor_onchecking;

    //successor's predecessor info (temporarily stored)
    ring_id_t predecessor_of_successor = -1;

    char predecessor_of_successor_ip[20];

    int predecessor_of_successor_port;

    /*
     * successor list (closest n successors)
     * currently maintain 10 elements for test
     * */
    //std::map<int, PeerPair> successor_list;
    std::list<PeerPair> successor_list;
    std::mutex _mtx;
    /*
     * service list (this node as a client)
     * currently a client can fetch one service at a time
     * */
    std::map<uint64_t, bool > client_service_list;
    /*
     * registered list (this node as a service provider)
     * currently a server can serve one client at a time
     * */
    std::map<ring_id_t, uint64_t> server_client_to_service;
    /*
     * content queue (this node as a client)
     * currently for one service package storage
     * */
    ConcurrentBlockQueue<const char*> package_queue;


    // figer table
    ChordFinger* finger_table;

    //debug
    uint64_t dummy_cnt = 0;
    uint64_t app_cnt = 0;
    uint64_t p2p_cnt = 0;
    int cnt_interval = 1; //s
    int recheck = 10;
    int checkwait = 2;
    int d_p2p_time_before = 10; //ms
    int d_p2p_time_after = 1; //s
    RelayConfig *config;


    ring_id_t find_closest_preceding_node(ring_id_t dest_id) {
        for (int i = m - 1;i >= 0;i--) {
            if (is_in_range(id, dest_id, finger_table[i].successor)) {
                return finger_table[i].successor;
            }
        }
        return this->id;
    }

    const char* find_closest_preceding_node_ip(ring_id_t dest_id) {
        for (int i = m - 1;i >= 0;i--) {
            if (is_in_range(id, dest_id, finger_table[i].successor)) {
                return finger_table[i].ip;
            }
        }
        return this->Node_ip;
    }

    int find_closest_preceding_node_port(ring_id_t dest_id) {
        for (int i = m - 1;i >= 0;i--) {
            if (is_in_range(id, dest_id, finger_table[i].successor)) {
                return finger_table[i].port;
            }
        }
        return this->port;
    }

    bool is_in_range(ring_id_t current_id, ring_id_t desti_id, ring_id_t successor_id) {

        if(current_id < desti_id){
            return current_id < successor_id && successor_id < desti_id;
        }else if(successor_id > current_id){
            return successor_id < (desti_id+ pow(2,this->m));
        }else{
            return successor_id < desti_id;
        }
    }

//    bool is_left_tight_in_range(ring_id_t left, ring_id_t right, ring_id_t val) {
//        if (left == val) return true;
//        // the range is the whole range
//        if (left == right) return true;
//        return is_in_range(left, right, val);
//    }

    bool is_right_tight_in_range(ring_id_t left, ring_id_t right, ring_id_t val) {
        if (right == val){
            return true;
        }else{
            return is_in_range(left, val, right);
        }
    }

public:
    // As a dead drop
    std::map<msg_info, std::unique_ptr<AnonymousPacket>> recv_list_deaddrop;
    // As a C/S
    //std::map<msg_info, std::unique_ptr<ConcurrentBlockQueue<std::unique_ptr<AnonymousPacket>>>> recv_list_user;
    ConcurrentBlockQueue<std::unique_ptr<AnonymousPacket>> recv_list_user;
    //msg queue
    ConcurrentBlockQueue<std::unique_ptr<MsgDescriptor>> MsgQueue;
    //service queue
    ConcurrentBlockQueue<std::unique_ptr<RelayConfig>> ServiceQueue;

    DAENode() = default;

    explicit DAENode(int _m, ring_id_t _id, int _port, const char* _addr, int bootstrap_t, RelayConfig *_config, DAEProtocol* _pro):m(_m), bootstrap_time(bootstrap_t){
        DLOG(INFO) << "in DAENode constructor!!!!!!";
        this->cnt_interval = _config->cnt_interval;
        this->recheck = _config->recheck;
        this->checkwait = _config->checkwait;
        this->d_p2p_time_after = _config->d_p2p_time_after;
        this->d_p2p_time_before = _config->d_p2p_time_before;
        this->config = _config;

        // current node initialize
        this->id = _id;
        strncpy(Node_ip, _addr, strlen(_addr) + 1);
        this->port = _port;

        // predecessor initialize
        this->predecessor = -1;
        strncpy(predecessor_ip, _addr, strlen(_addr) + 1);
        this->predecessor_port = _port;

        //for failure checking
        this->predecessor_status = false;
        this->successor_status = false;
        this->predecessor_onchecking = false;
        this->successor_onchecking = false;

        // finger table initialize
        finger_table = new ChordFinger[_m];
        for (int i = 0;i < m;i++) {
            finger_table[i].start = (id + (1 << i)) % (1 << m);
            finger_table[i].to = (id + (1 << (i + 1))) % (1 << m);
            finger_table[i].successor = id;
            strncpy(finger_table[i].ip, _addr, strlen(_addr) + 1);
            finger_table[i].port = _port;
        }

        // protocol initialize
        protocol = _pro;
    }

//    void add_to_swaper(ring_id_t id, const char* str){
//        char* p_value = new char[MAX_MTU];
//        strncpy(p_value, str, strlen(str) + 1);
//        this->swaper[id] = p_value;
//    }
//
//    bool is_in_swaper(ring_id_t id){
//        if(swaper.count(id)){
//            return true;
//        }else{
//            return false;
//        }
//    }
//
//    int get_queue_size(){
//        return this->package_queue.size();
//    }

    bool check_existence(int session, int seq){
        msg_info msg = std::make_tuple(session, seq);
        return recv_list_deaddrop.count(msg) != 0;
    }


    const char* getContent(){
        return this->package_queue.pop_front();
    }

    void set_server_list(ring_id_t id, uint64_t service){
        this->server_client_to_service[id] = service;
    }

    bool is_client_registered(ring_id_t id, uint64_t service){

        if(this->server_client_to_service[id] == service){
            return true;
        }else{
            return false;
        }
    }

    void set_client_service_list(uint64_t service, bool connect_or_not){
        this->client_service_list[service] = connect_or_not;
    }

    bool is_service_connected(uint64_t service){
        return this->client_service_list[service];
    }

    void add_to_successor_list(ring_id_t id, PeerPair& peer){

        PeerPair backup_successor = {.ip = peer.ip, .port = peer.port, .relay_id = peer.relay_id};

        this->successor_list.push_back(backup_successor);
    }

    ring_id_t fetch_id_from_successor_list(){
        return this->successor_list.front().relay_id;
    }

    std::string fetch_ip_from_successor_list(){
        return this->successor_list.front().ip;
    }

    int fetch_port_from_successor_list(){
        return this->successor_list.front().port;
    }

    bool fetch_usage_from_successor_list(){
        return this->successor_list.front().check_usage;
    }

    void set_predecessor_of_successor(ring_id_t id){
        this->predecessor_of_successor = id;
    }

    void set_predecessor_of_successor_ip(const char* ip){
        strncpy(this->predecessor_of_successor_ip, ip, strlen(ip) + 1);
    }

    void set_predecessor_of_successor_port(int _port){
        this->predecessor_of_successor_port = _port;
    }

    int get_entry_num(){
        return this->m;
    }

    const char* getFingerIP(int k) {
        return finger_table[k].ip;
    }

    int getFingerport(int k) {
        return finger_table[k].port;
    }

    const char* getNodeIP() {
        return this->Node_ip;
    }

    ring_id_t getid(){
        return this->id;
    }

    ring_id_t getNodeport(){
        return this->port;
    }

    void updatesuccessor(int k, ring_id_t su) {
        this->finger_table[k].successor = su;
    }

    void updatesuccessor_ip(int k, const char* _ip) {
        strncpy(this->finger_table[k].ip, _ip, strlen(_ip) + 1);
    }

    void updatesuccessor_port(int k, int _port) {
        this->finger_table[k].port = _port;
    }

    ring_id_t successor() {
        return finger_table[0].successor;
    }

    void setpredecessor(ring_id_t id) {
        this->predecessor = id;
    }

    ring_id_t getpredecessor() {
        return this->predecessor;
    }

    void setpredecessor_ip(const char* ip) {
        strncpy(this->predecessor_ip, ip, strlen(ip) + 1);
    }

    void setpredecessor_port(int _port) {
        this->predecessor_port = _port;
    }

    ring_id_t find_successor_from_fingertable(ring_id_t index) {
        return finger_table[index].successor;
    }

    void reverse_predecessor_status(){
        this->predecessor_status = true;
    }

    void reverse_successor_status(){
        this->successor_status = true;
    }

    ring_id_t find_successor(const char* _src_ip, int _src_port, int index, ring_id_t dest_id, bool &is_find);

    void handle_find_successor(const char* _src_ip, int _src_port, int index, ring_id_t dest_id, bool &is_find);

    void handle_get_predecessor(const char* _src_ip, int _src_port);

    //create a new Chord ring
    void create();

    void join_with_help(const PeerPair &joiner_node);

    // join a Chord ring containing node join_node
    void join(const PeerPair &joiner_node, bool &is_find);

    //called periodically, verify n's immediate successor, and tells the successor about joining node
    void stabilize();

    //notify the coming of possible predecessor
    void notify(ring_id_t possible_predecessor, const char* _ip, int _port);

    //called periodically, refreshes fingertable entries, store the index
    void fix_fingers();

    //called periodically, check predecessor failure. If true, set predecessor = -1
    void check_predecessor_failure();

    void handle_check_predecessor_failure(const char* src_ip, int src_port);

    //called periodically, check successor failure. If true, 1. set successor = Node_id; 2. set successor.info = std::map.begin().info
    void check_successor_failure();

    void handle_check_successor_failure(const char* src_ip, int src_port);

    // leave the network
    void leave();

    // info
    void print_info();

    void fetch_successor_list();

    void handle_fetch_successor_list(int counter, const char* src_ip, int src_port);

    inline ChordFinger* chordFinger() { return finger_table; }


    //external service
    void connect_service(uint64_t msg_type, ring_id_t connector_id, ring_id_t client_key);

    void handle_external_messaging(bool is_deaddrop, int session, int seq, uint64_t msg_type, ring_id_t dest_id, ring_id_t client_key, const char* contect_package, int size);

    void send_content_package(bool is_deaddrop, int session, int seq, uint64_t msg_type, ring_id_t connector_id, ring_id_t client_key, char *buf, int size);

    void pop_msg(){
        //this->handle_external_messaging(is_dd, session, seq, msg_type, connector_id, client_key, buf, size);
        while(1){
            std::unique_ptr<MsgDescriptor> Msg = std::move(this->MsgQueue.pop_front());

            DLOG(INFO) << "DAENode " << this->id << "pop msg from buffer" ;

            this->send_content_package(Msg->info.is_deaddrop,
                                       Msg->info.session_id,
                                       Msg->info.seq_id,
                                       Msg->info.msgtype,
                                       Msg->info.to_id,
                                       Msg->info.from_id,
                                       Msg->info.msg,
                                       Msg->info.size_of_payload);
        }
    }

    void print_msg(){
//        DLOG(INFO) << "Node " << this->id << " recv-msg-queue is of size: " << this->recv_list_deaddrop.size();
//        for(auto iter = this->recv_list_deaddrop.begin(); iter != this->recv_list_deaddrop.end(); iter++)
//        {
//            DLOG(INFO) << "Here is a in-queue recved msg info:";
//            DLOG(INFO) << "key: session " <<  iter->first.session << "seq " << iter->first.seq;
//            std::unique_ptr<AnonymousPacket> packet = std::move(iter->second->pop_front());
//            DLOG(INFO) << "value: ";
//            DLOG(INFO) << packet->buf;
//        }
    }
};


#endif //ANONYMOUSP2P_DAENODE_HPP
