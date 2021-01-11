#include <message/message.hpp>
#include "DAENode.hpp"
#include "DAEProtocol.hpp"
#include "utils/EventTask.hpp"

ring_id_t DAENode::find_successor(const char* _src_ip, int port, int index, ring_id_t dest_id, bool &is_find) {

    this->handle_find_successor(_src_ip, port, index, dest_id, is_find);
}

void DAENode::handle_find_successor(const char* _src_ip, int _src_port, int index, ring_id_t dest_id, bool &is_find) {
    if(this->id == dest_id){
        const char* addr = this->Node_ip;
        int su_port = this->port;
        is_find = true;
        this->protocol->reply_find_successor(_src_ip, _src_port, dest_id, index, this->id, addr, su_port, is_find);
        //DLOG(INFO) << "In handle_find_successor, the su ip is: " << addr << "port is: " << port;
    }else if(is_right_tight_in_range(this->id, dest_id, this->successor())){
        const char* addr = this->finger_table[0].ip;
        int su_port = this->finger_table[0].port;
        is_find = true;
        this->protocol->reply_find_successor(_src_ip, _src_port, dest_id, index, this->successor(), addr, su_port, is_find);
        //DLOG(INFO) << "In handle_find_successor, the su ip is: " << addr << "port is: " << port;
    }else{
        ring_id_t closest_preceding_node = find_closest_preceding_node(dest_id);
        const char* closest_preceding_node_addr = find_closest_preceding_node_ip(dest_id);
        int closest_preceding_node_port = find_closest_preceding_node_port(dest_id);
        if(closest_preceding_node == this->id){
            this->protocol->reply_find_successor(_src_ip, _src_port, dest_id, index, this->finger_table[0].successor,
                                                 this->finger_table[0].ip, this->finger_table[0].port, is_find);
            //DLOG(INFO) << "In handle_find_successor, the su ip is: " << closest_preceding_node_addr << "port is: " << closest_preceding_node_port;
        }else{
            this->protocol->join_find_successor(closest_preceding_node_addr, closest_preceding_node, closest_preceding_node_port,
                    _src_ip, -1, _src_port, index, dest_id, is_find);
        }
    }
}

void DAENode::handle_external_messaging(bool is_deaddrop, int session, int seq, uint64_t msg_type, ring_id_t dest_id, ring_id_t client_key, const char* ct_package, int size){
    DLOG(INFO) << this->id << " in handle_external_messaging" << "from " << client_key << "to " << dest_id << "type " <<msg_type;
    DLOG(INFO) << "an external sending info: " << ct_package;

    if(this->id == dest_id){
        const char* addr = this->Node_ip;
        int su_port = this->port;
        this->protocol->external_messaging(true, session, seq, this->id, addr, su_port, msg_type, dest_id, client_key, ct_package, size);
    }else if(is_right_tight_in_range(this->id, dest_id, this->successor())){
        const char* addr = this->finger_table[0].ip;
        int su_port = this->finger_table[0].port;
        this->protocol->external_messaging(true, session, seq, this->successor(), addr, su_port, msg_type, dest_id, client_key, ct_package, size);
        DLOG(INFO) << "In handle_external_messaging, send to node: " << this->successor() << "ip: " << addr;
    }else{
        ring_id_t closest_preceding_node = find_closest_preceding_node(dest_id);
        const char* closest_preceding_node_addr = find_closest_preceding_node_ip(dest_id);
        int closest_preceding_node_port = find_closest_preceding_node_port(dest_id);
        if(closest_preceding_node == this->id){
            const char* addr = this->finger_table[0].ip;
            int su_port = this->finger_table[0].port;
            this->protocol->external_messaging(true, session, seq, this->successor(), addr, su_port, msg_type, dest_id, client_key, ct_package, size);
            DLOG(INFO) << "In handle_external_messaging, send to node: " << this->successor() << "ip: " << addr;
        }else{
            this->protocol->external_messaging(false, session, seq, closest_preceding_node, closest_preceding_node_addr, closest_preceding_node_port,
                    msg_type, dest_id, client_key, ct_package, size);
            DLOG(INFO) << "In handle_external_messaging, send to node: " << closest_preceding_node << "ip: " << closest_preceding_node_addr;
        }
    }
}

void DAENode::handle_get_predecessor(const char* _src_ip, int _src_port) {
    this->protocol->reply_get_predecessor(_src_ip, _src_port, this->getpredecessor(), this->predecessor_ip, this->predecessor_port);
}

//void DAENode::create(){
//    this->predecessor = -1;
//    this->finger_table[0].successor = this->id;
//
//    std::thread t([this](){
//        while(1){
//            sleep(1);
//            DLOG(INFO) << this->id << "is running protocol: create " ;
//            this->stabilize();
//            this->fix_fingers();
//            this->check_predecessor();
//        }
//    });
////        t.join();
//    t.detach();
//}


static DAENode* current_node;
PeerPair joiner;

void DAENode::join_with_help(const PeerPair &joiner_node) {
    bool is_find = false;
    std::cout << "join\n";
    this->protocol->join_find_successor(joiner_node.ip.c_str(), joiner_node.relay_id,
                                        joiner_node.port, this->Node_ip, this->id, this->port, 0, this->id, is_find);
    struct timeval t = {
        .tv_sec = 2,
        .tv_usec = 0
    };
    EventTask::global->future_execute(t, [](){
        if (current_node->finger_table[0].successor == current_node->id) { current_node->join_with_help(joiner); }
    });
}

typeof(std::chrono::steady_clock::now()) start;
bool is_bootstrap = true;

void DAENode::join(const PeerPair &joiner_node, bool &is_find){

    joiner = joiner_node;
    current_node = this;
    // if not a joiner node, then call for join_find_successor with a time-out
    if(strcmp(this->Node_ip, joiner_node.ip.c_str()) != 0 || this->port != joiner_node.port) {
        join_with_help(joiner_node);
    }

    this->stabilize();
    this->fix_fingers();
    this->fetch_successor_list();

    struct timeval time_internal = {
        .tv_sec = 0,
        .tv_usec = 1000 * this->d_p2p_time_before
    };

    start = std::chrono::steady_clock::now();
    EventTask::global->add_time_task_lambda(time_internal, [this](struct timeval *next){
        if (is_bootstrap) {
            auto now = std::chrono::steady_clock::now();
            auto dur = std::chrono::duration_cast<std::chrono::seconds>
                    (now - start);
            if (dur.count() > bootstrap_time) {
                is_bootstrap = false;
                next->tv_sec = this->d_p2p_time_after;
                next->tv_usec = 0;
            }
        }
        stabilize();
        fix_fingers();
        if(this->config->check_failure){
            check_predecessor_failure();
            check_successor_failure();
        }
        print_info();
    });
    if(config->check_failure){
        struct timeval tv = {
            .tv_sec = 10,
            .tv_usec = 0
        };
        EventTask::global->add_time_task_lambda(tv, [this](struct timeval *tv){
            fetch_successor_list();
        });
    }
}

void DAENode::stabilize(){
    //DLOG(INFO) << "Start stabilize " ;
//    if(this->successor() != this->id){
//        this->protocol->find_predecessor(this->finger_table[0].ip, this->id, this->Node_ip, this->successor());
//    }
    this->protocol->find_predecessor(this->finger_table[0].ip, this->finger_table[0].port, this->id, this->Node_ip, this->port, this->successor());
    //DLOG(INFO) << this->id << " in stabilize, predecessor is " << this-> predecessor_of_successor;
    if(is_in_range(this->id, this->successor(), this->predecessor_of_successor)){
        this->updatesuccessor(0, this->predecessor_of_successor);
        this->updatesuccessor_ip(0, this->predecessor_of_successor_ip);
        this->updatesuccessor_port(0, this->predecessor_of_successor_port);
        if(this->predecessor_of_successor == 1){
            //DLOG(INFO) << "in stabilize find change ft operation";
            //DLOG(INFO) << "su ip: " << this->predecessor_of_successor_ip;
        }
    }
    if(this->successor() != this->id){
        this->protocol->notify_predecessor_join(this->finger_table[0].ip, this->successor(), this->finger_table[0].port, this->id);
    }
    //DLOG(INFO) << "End stabilize " ;
}

void DAENode::notify(ring_id_t possible_predecessor, const char* possible_predecessor_ip, int possible_predecessor_port){
    if(this->getpredecessor() == -1 || is_in_range(this->getpredecessor(), this->id, possible_predecessor)){
        this->setpredecessor(possible_predecessor);
        this->setpredecessor_ip(possible_predecessor_ip);
        this->setpredecessor_port(possible_predecessor_port);
        //DLOG(INFO) << this->getid() << "sets predecessor as: " << possible_predecessor;
    }
}

void DAENode::fix_fingers(){
    //DLOG(INFO) << "in fix_fingers()" ;
    bool check = false;
    this->find_successor(this->Node_ip, this->port, 0, (this->finger_table[0].start), check);
    for(int i = 0; i < this->get_entry_num() - 1; i++){
        if(is_in_range(this->id, finger_table[i].successor ,finger_table[i+1].start)){
            updatesuccessor(i+1, finger_table[i].successor);
            updatesuccessor_ip(i+1, finger_table[i].ip);
            updatesuccessor_port(i+1, finger_table[i].port);
        }else{
            check = false;
            this->find_successor(this->Node_ip, this->port, i+1, (this->finger_table[i+1].start), check);
        }

    }
}

void DAENode::check_predecessor_failure(){
    if(!this->predecessor_onchecking && this->predecessor!= -1){
        this->predecessor_onchecking = true;
        // save current bool value
        this->predecessor_status = false;

        // send & wait for reply, once replied, change status to the opposite
        for(int i = 1; i <= this->recheck; i++){
            struct timeval tv = {
                .tv_sec = 0,
                .tv_usec = i*50*1000
            };
            EventTask::global->future_execute_lambda(tv, [this](){
                this->protocol->check_predecessor_existence(this->predecessor_ip, this->predecessor_port, this->Node_ip, this->port);
            });
        }

        struct timeval tv = {
            .tv_sec = this->checkwait,
            .tv_usec = 0
        };

        EventTask::global->future_execute(tv, [](){
            if (!current_node->predecessor_status) {
                current_node->predecessor = -1;
            }
            current_node->predecessor_onchecking = false;
        });

    }

}

void DAENode::check_successor_failure(){
    if(!this->successor_onchecking && this->finger_table[0].successor != this->id){
        this->successor_onchecking = true;
        // save current bool value
        this->successor_status = false;

        // send & wait for reply, once replied, change status to the opposite
        for(int i = 1; i <= this->recheck; i++){
            struct timeval tv = {
                .tv_sec = 0,
                .tv_usec = i*50*1000
            };
            EventTask::global->future_execute_lambda(tv, [this](){
                this->protocol->check_successor_existence(this->finger_table[0].ip, this->finger_table[0].port, this->Node_ip, this->port);
            });
        }

        struct timeval tv = {
            .tv_sec = this->checkwait,
            .tv_usec = 0
        };
        EventTask::global->future_execute(tv, [](){
            if (!current_node->successor_status) {
                std::lock_guard<std::mutex> lck(current_node->_mtx);
                if (current_node->successor_list.size() == 0) {
                    LOG(ERROR) << "empty successor list";
                    current_node->successor_onchecking = false;
                    return;
                }
                auto head = current_node->successor_list.front();
                if(head.relay_id == current_node->finger_table[0].successor){
                    current_node->successor_list.pop_front();
                }
                if (current_node->successor_list.size() == 0) {
                    LOG(ERROR) << "empty successor list";
                    current_node->successor_onchecking = false;
                    return;
                }
                DLOG(INFO) << "successor list not empty, size = " << current_node->successor_list.size();
                ring_id_t successor = current_node->successor_list.front().relay_id;
                const char* successor_ip = current_node->successor_list.front().ip.c_str();
                int successor_port = current_node->successor_list.front().port;

                current_node->updatesuccessor(0, successor);
                current_node->updatesuccessor_ip(0, successor_ip);
                current_node->updatesuccessor_port(0, successor_port);
            }
            current_node->successor_onchecking = false;
        });
    }
}

void DAENode::handle_check_predecessor_failure(const char* src_ip, int src_port){
    this->protocol->reply_predecessor_existence(this->id, src_ip, src_port);
}

void DAENode::handle_check_successor_failure(const char* src_ip, int src_port){
    this->protocol->reply_successor_existence(this->id, src_ip, src_port);
}

void DAENode::print_info() {
    struct timeval tv7;
    gettimeofday(&tv7,NULL);
    DLOG(INFO) << "time: start printing info: " << tv7.tv_sec*1000 + tv7.tv_usec/1000;
    DLOG(INFO) << "printing routing info for node " << id;
    DLOG(INFO) << "current predecessor is" << predecessor;
    for (int i = 0;i < m;i++) {
        DLOG(INFO) << finger_table[i].start << ", " << finger_table[i].successor << ", "
        << finger_table[i].ip << ", " << finger_table[i].port << "\n";
    }
    DLOG(INFO) << "printing routing info for node " << id << " successor list:";
    DLOG(INFO) << " successor list size :" << successor_list.size();
    for (auto & su : successor_list) {
        DLOG(INFO) << su.relay_id<< ", " << su.ip << ", "
                  << su.port << ", " << su.check_usage << "\n";
    }

    this->print_msg();
}

void DAENode::fetch_successor_list(){
    // Currently ask for 5 backup successors in case of failure
    int backup_num = 6;
    this->protocol->fetch_backup_successor(backup_num - 1, this->finger_table[0].ip, this->finger_table[0].port, this->id, this->Node_ip, this->port);

}

void DAENode::handle_fetch_successor_list(int counter, const char* src_ip, int src_port){

    // 1. reply to source
    this->protocol->reply_backup_successor(counter, src_ip, src_port, this->finger_table[0].successor, this->finger_table[0].ip, this->finger_table[0].port);
    // 2. if counter > 0, then send to my successor
    if(counter > 0){
        counter--;
        this->protocol->fetch_backup_successor(counter, this->finger_table[0].ip, this->finger_table[0].port, -1, src_ip, src_port);
    }

}

void DAENode::connect_service(uint64_t msg_type, ring_id_t connector_id, ring_id_t client_key){
    this->set_client_service_list(msg_type, false);
    // temporarily: payload size = 0
    this->handle_external_messaging(false, 0, 0, msg_type, connector_id, client_key, "CONNECT", 0);
}

void DAENode::send_content_package(bool is_dd, int session, int seq, uint64_t msg_type, ring_id_t connector_id, ring_id_t client_key, char *buf, int size){
    this->handle_external_messaging(is_dd, session, seq, msg_type, connector_id, client_key, buf, size);
}

