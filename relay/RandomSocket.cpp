//
// Created by Maxxie Jiang on 28/1/2019.
//

#include "Sharp.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <thread>
#include <obfuscation/ShufflePool.hpp>
#include <obfuscation/DirectPool.hpp>
#include "RandomSocket.hpp"
#include "message/message.hpp"
#include "SocketHeader.hpp"
#include "utils/EventTask.hpp"
#include <unistd.h>

//void RandomSocket::direct_send(std::unique_ptr<AnonymousPacket> packet, host_port_t &dest) {
//    int t = sendto(socket_fd, (void*)(packet.get()), packet->total_size(), 0, (struct sockaddr*)&dest.sock, sizeof(struct sockaddr));
//    if (t == -1) {
//        ;
//    }
////    LOG(INFO) << "sendto " << dest.host << ":" << dest.port << " " << packet->total_size();
//}

void RandomSocket::DAE_direct_send(std::unique_ptr<AnonymousPacket> packet, const char* _ip_addr, int dest_port) {

    _ip_addr = packet->header()->relay_dest_ip;
    dest_port = packet->header()->relay_dest_port;

    struct sockaddr_in server_addr;

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    //just for local test
    DLOG(INFO) << this->relay_id << "in Direct send, the packet is from " << packet->header()->relay_id;

    server_addr.sin_port = htons(dest_port);
    server_addr.sin_addr.s_addr = inet_addr(_ip_addr);

    int t = sendto(socket_fd, (void*)(packet.get()), packet->total_size(), 0, (const struct sockaddr *) &server_addr, sizeof(server_addr));
    if (t == -1) {
        LOG(ERROR) << "fail to send in DAE_direct_send";
    }
    if(packet->is_internal_msg() && packet->header()->id != message::dummy_message) {
        this->in_socket_node_instance->p2p_cnt += 1;
    }

    DLOG(INFO) << "DAE_direct_send:" << packet->header()->relay_ip << "-->" << _ip_addr << ", service type: " << packet->header()->id;
}

RandomSocket::RandomSocket(std::string _host, int _port, uint64_t _relay_id, DAENode* node, const RelayConfig &fig) {
    host = _host;
    port = _port;
    relay_id = _relay_id;
    in_socket_node_instance = node;
    config = fig;
}

RandomSocket *current_socket = NULL;

bool RandomSocket::init_DAEService() {

    current_socket = this;

    LOG(INFO) << "init socket with " << host << ":" << port << ", DAENode ID:" << relay_id;

    struct sockaddr_in server_addr;

    if ( (socket_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        LOG(ERROR) << "UDP socket creation failed";
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family =   AF_INET;
    server_addr.sin_port   =   htons(port);
    server_addr.sin_addr.s_addr    =   htonl(INADDR_ANY);

    // Bind the socket with the server address
    if ( bind(socket_fd, (const struct sockaddr *)&server_addr,
              sizeof(server_addr)) < 0 )
    {
        LOG(ERROR) << "bind failed";
        exit(EXIT_FAILURE);
    }

	std::thread([&](){
        socklen_t sock_len;
        struct sockaddr_in client_addr;
	while(true) {
        auto r_packet = std::make_unique<AnonymousPacket>();
        int size = recvfrom(socket_fd, (void*)r_packet->buf, MAX_MTU, 0, ( struct sockaddr *) &client_addr, &sock_len);

        if (size <= 0) {
            // LOG(ERROR) << "connection close or reset";
            return;
        }

        DLOG(INFO) << "r_packet->header()->id" << r_packet->header()->id;
        if(r_packet->is_internal_msg()){
            DLOG(INFO) << current_socket->in_socket_node_instance->getid() << " is processing *internal* msg...";
            //this->in_socket_node_instance->print_info();
            switch (r_packet->header()->id) {
                case message::internal_join_find_successor: {
                    DLOG(INFO) << current_socket->in_socket_node_instance->getid() << " recv a join_find_successor packet from " << r_packet->header()->relay_ip;
                    uint64_t id = r_packet->header()->id;
                    current_socket->DAE_process_internal_message(id, std::move(r_packet));
                    break;
                }
                case message::internal_reply_find_successor: {
                    DLOG(INFO) << current_socket->in_socket_node_instance->getid() << " recv a reply_find_successor packet from " << r_packet->header()->relay_ip;
                    uint64_t id = r_packet->header()->id;
                    //this->in_socket_node_instance->print_info();
                    current_socket->DAE_process_internal_message(id, std::move(r_packet));
                    break;
                }
                case message::internal_find_predecessor: {
                    DLOG(INFO) << current_socket->in_socket_node_instance->getid() << " recv a find_predecessor packet";
                    uint64_t id = r_packet->header()->id;
                    current_socket->DAE_process_internal_message(id, std::move(r_packet));
                    break;
                }
                case message::internal_reply_get_predecessor: {
                    DLOG(INFO) << current_socket->in_socket_node_instance->getid() << " recv a reply_get_predecessor packet";
                    uint64_t id = r_packet->header()->id;
                    current_socket->DAE_process_internal_message(id, std::move(r_packet));
                    break;
                }
                case message::internal_notify_predecessor_join: {
                    DLOG(INFO) << current_socket->in_socket_node_instance->getid() << " recv a notify_predecessor_join packet";
                    uint64_t id = r_packet->header()->id;
                    current_socket->DAE_process_internal_message(id, std::move(r_packet));
                    break;
                }
                case message::internal_fetch_successor_list: {
                    DLOG(INFO) << current_socket->in_socket_node_instance->getid() << " recv a internal_fetch_successor_list packet";
                    uint64_t id = r_packet->header()->id;
                    current_socket->DAE_process_internal_message(id, std::move(r_packet));
                    break;
                }
                case message::internal_reply_successor_list: {
                    DLOG(INFO) << current_socket->in_socket_node_instance->getid() << " recv a internal_reply_successor_list packet";
                    uint64_t id = r_packet->header()->id;
                    current_socket->DAE_process_internal_message(id, std::move(r_packet));
                    break;
                }
                case message::internal_check_predecessor_existence: {
                    DLOG(INFO) << current_socket->in_socket_node_instance->getid() << " recv a internal_check_predecessor_existence packet";
                    uint64_t id = r_packet->header()->id;
                    current_socket->DAE_process_internal_message(id, std::move(r_packet));
                    break;
                }
                case message::internal_reply_predecessor_existence: {
                    DLOG(INFO) << current_socket->in_socket_node_instance->getid() << " recv a internal_reply_predecessor_existence packet";
                    uint64_t id = r_packet->header()->id;
                    current_socket->DAE_process_internal_message(id, std::move(r_packet));
                    break;
                }
                case message::internal_check_successor_existence: {
                    DLOG(INFO) << current_socket->in_socket_node_instance->getid() << " recv a internal_check_successor_existence packet";
                    uint64_t id = r_packet->header()->id;
                    current_socket->DAE_process_internal_message(id, std::move(r_packet));
                    break;
                }
                case message::internal_reply_successor_existence: {
                    DLOG(INFO) << current_socket->in_socket_node_instance->getid() << " recv a internal_reply_successor_existence packet";
                    uint64_t id = r_packet->header()->id;
                    current_socket->DAE_process_internal_message(id, std::move(r_packet));
                    break;
                }
                case message::dummy_message: {
                    DLOG(INFO) << current_socket->in_socket_node_instance->getid() << " get dummy message";
                    uint64_t id = r_packet->header()->id;
                    current_socket->DAE_process_internal_message(id, std::move(r_packet));
                    break;
                }
                default:
                    LOG(INFO) << "in internal default, " << "recv a packet to: " << r_packet->header()->relay_dest_id;
                    uint64_t id = r_packet->header()->id;
                    // recv_list[id]->push(std::move(r_packet));
            }
        } else {
            DLOG(INFO) << "Processing *external* msg...";
            switch (r_packet->header()->id){
                case message::external_pingpong_service: {
                    DLOG(INFO) << current_socket->in_socket_node_instance->getid() << " recv an external_pingpong_service packet";
                    uint64_t id = r_packet->header()->id;
                    current_socket->DAE_process_internal_message(id, std::move(r_packet));
                    break;
                }
                case message::external_reply_pingpong_service: {
                    DLOG(INFO) << current_socket->in_socket_node_instance->getid() << " recv an external_reply_pingpong_service packet";
                    uint64_t id = r_packet->header()->id;
                    current_socket->DAE_process_internal_message(id, std::move(r_packet));
                    break;
                }
                case message::external_deaddrop_messaging: {
                    DLOG(INFO) << current_socket->in_socket_node_instance->getid() << " recv an external_deaddrop_messaging packet";
                    uint64_t id = r_packet->header()->id;
                    current_socket->DAE_process_internal_message(id, std::move(r_packet));
                    break;
                }
                case message::external_reply_deaddrop_messaging: {
                    DLOG(INFO) << current_socket->in_socket_node_instance->getid() << " recv an external_reply_deaddrop_messaging packet";
                    uint64_t id = r_packet->header()->id;
                    current_socket->DAE_process_internal_message(id, std::move(r_packet));
                    break;
                }
                default:
                    LOG(ERROR) << "in external default, " << "recv a packet to: " << r_packet->header()->relay_dest_id;
            }
            //safe_send(std::move(r_packet));
        }
	}
    }).detach();

    LOG(INFO) << in_socket_node_instance->getid()  << " init random socket successfully.";

    if (config.shuffle == "shuffle") {
        pool = new ShufflePool(this, in_socket_node_instance->chordFinger(), in_socket_node_instance->m, config.msg_rate, config.min_pool, config.debug_shuffle, config.shuffle_P, config.send_dummy);
    } else {
        pool = new DirectPool(this, config.msg_rate);
    }
    struct timeval tv = {
        .tv_sec = this->in_socket_node_instance->bootstrap_time ,
        .tv_usec = 0
    };
    EventTask::global->future_execute_lambda(tv, [this](){
        pool->start();
    });

    return true;
}

void RandomSocket::DAE_process_internal_message(uint64_t id, std::unique_ptr<AnonymousPacket> packet) {
    // Extract necessary packet info, re-package and send out
    switch (id) {

        case message::internal_join_find_successor: {

            DLOG(INFO) << in_socket_node_instance->getid()  << "parsing a join_find_successor packet";
            //void DAENode::handle_find_successor(const char* _src_ip, int index, ring_id_t dest_id, bool &is_find)

            const char* source_addr = (packet->header()->relay_reply_ip);

            int source_port = packet->header()->relay_reply_port;

            int index = packet->header()->index;

            ring_id_t desti_id = packet->header()->key_id;

            bool check = false;
            DLOG(INFO) << "parse: reply ip: " << source_addr ;
            //this->in_socket_node_instance->print_info();
            this->in_socket_node_instance->handle_find_successor(source_addr, source_port, index, desti_id, check);

            break;
        }

        case message::internal_reply_find_successor: {
            //this->in_socket_node_instance->print_info();

            DLOG(INFO) << in_socket_node_instance->getid()  << "parsing a reply_find_successor packet, su's ip is ";
            //<<packet->header()->key_ip;

            int index = packet->header()->index;

            ring_id_t successor = packet->header()->key_id;

            const char* successor_ip = packet->header()->key_ip;

            int successor_port = packet->header()->key_port;

            this->in_socket_node_instance->updatesuccessor(index, successor);
            this->in_socket_node_instance->updatesuccessor_ip(index, successor_ip);
            this->in_socket_node_instance->updatesuccessor_port(index, successor_port);

            DLOG(INFO) << "Update successor info! su's ip is: " << successor_ip << "port is: " << successor_port;

            //this->in_socket_node_instance->print_info();

            break;
        }

        case message::internal_find_predecessor: {

            DLOG(INFO) << in_socket_node_instance->getid()  << "parsing a find_predecessor packet";

            const char* source_addr = (packet->header()->relay_reply_ip);

            int source_port = packet->header()->relay_reply_port;

            this->in_socket_node_instance->handle_get_predecessor(source_addr, source_port);

            break;
        }

        case message::internal_reply_get_predecessor: {

            DLOG(INFO) << in_socket_node_instance->getid()  << "parsing a reply_get_predecessor packet";

            ring_id_t predecessor_of_successor = packet->header()->key_id;

            const char* predecessor_of_successor_ip = (packet->header()->key_ip);

            int predecessor_of_successor_port = packet->header()->key_port;

            DLOG(INFO) << "the predecessor_of_successor is: " << predecessor_of_successor;
            DLOG(INFO) << "the predecessor_of_successor_ip is: " << predecessor_of_successor_ip;
            DLOG(INFO) << "the predecessor_of_successor_port is: " << predecessor_of_successor_port;
            this->in_socket_node_instance->set_predecessor_of_successor(predecessor_of_successor);

            this->in_socket_node_instance->set_predecessor_of_successor_ip(predecessor_of_successor_ip);

            this->in_socket_node_instance->set_predecessor_of_successor_port(predecessor_of_successor_port);

            DLOG(INFO) << "Complete: parsing a reply_get_predecessor packet";

            break;
        }

        case message::internal_notify_predecessor_join: {

            DLOG(INFO) << in_socket_node_instance->getid()  << "parsing a notify_predecessor_join packet";
            //void DAENode::notify(ring_id_t possible_predecessor)

            ring_id_t possible_predecessor = packet->header()->key_id;

            const char* possible_predecessor_ip = packet->header()->relay_ip;

            int possible_predecessor_port = packet->header()->key_port;

            this->in_socket_node_instance->notify(possible_predecessor, possible_predecessor_ip, possible_predecessor_port);

            break;
        }

        case message::internal_fetch_successor_list: {

            DLOG(INFO) << in_socket_node_instance->getid()  << "parsing a internal_fetch_successor_list packet";

            int counter = packet->header()->index;

            const char* source_ip = packet->header()->relay_reply_ip;

            int source_port = packet->header()->relay_reply_port;

            this->in_socket_node_instance->handle_fetch_successor_list(counter, source_ip, source_port);

            break;
        }

        case message::internal_reply_successor_list: {

            DLOG(INFO) << in_socket_node_instance->getid()  << "parsing a internal_reply_successor_list packet";

            ring_id_t su_id = packet->header()->key_id;
            const char* su_ip = packet->header()->key_ip;
            int su_port = packet->header()->key_port;
            int counter = packet->header()->index;

            if(su_id != this->in_socket_node_instance->id){
                std::lock_guard<std::mutex> lck(this->in_socket_node_instance->_mtx);
                PeerPair backup_successor = {.ip = su_ip, .port = su_port, .relay_id = su_id};
                this->in_socket_node_instance->add_to_successor_list(su_id, backup_successor);
                while(this->in_socket_node_instance->successor_list.size() > 6 - counter){
                    this->in_socket_node_instance->successor_list.pop_front();
                }
            }
            break;
        }

        case message::internal_check_predecessor_existence: {

            DLOG(INFO) << in_socket_node_instance->getid()  << "parsing a internal_check_predecessor_existence packet";

            const char* src_ip = packet->header()->relay_reply_ip;
            int src_port = packet->header()->relay_reply_port;

            this->in_socket_node_instance->handle_check_predecessor_failure(src_ip, src_port);

            break;
        }

        case message::internal_reply_predecessor_existence: {

            DLOG(INFO) << in_socket_node_instance->getid()  << "parsing a internal_reply_predecessor_existence packet";

            ring_id_t pre_id = packet->header()->key_id;

            if (pre_id == this->in_socket_node_instance->getpredecessor()){
                this->in_socket_node_instance->reverse_predecessor_status();
            }

            break;
        }

        case message::internal_check_successor_existence: {

            DLOG(INFO) << in_socket_node_instance->getid()  << "parsing a internal_check_successor_existence packet";

            const char* src_ip = packet->header()->relay_reply_ip;
            int src_port = packet->header()->relay_reply_port;

            this->in_socket_node_instance->handle_check_successor_failure(src_ip, src_port);

            break;
        }

        case message::internal_reply_successor_existence: {

            DLOG(INFO) << in_socket_node_instance->getid()  << "parsing a internal_reply_predecessor_existence packet";

            ring_id_t su_id = packet->header()->key_id;

            if (su_id == this->in_socket_node_instance->successor() && su_id != this->in_socket_node_instance->getid()){
                this->in_socket_node_instance->reverse_successor_status();
            }

            break;
        }

        case message::external_pingpong_service: {

            DLOG(INFO) << in_socket_node_instance->getid()  << "parsing a external_pingpong_service packet";
            int session_id = packet->header()->session_id;
            int s_id = packet->header()->index;
            uint64_t type = packet->header()->id;
            ring_id_t dest_id = packet->header()->key_id;
            ring_id_t client_key = packet->header()->relay_reply_id;

            this->in_socket_node_instance->handle_external_messaging(false, session_id, s_id, type, dest_id, client_key, packet->payload(), packet->payload_size());

            break;
        }

        case message::external_reply_pingpong_service: {

            DLOG(INFO) << in_socket_node_instance->getid()  << "parsing a external_reply_pingpong_service packet";
            int session_id = packet->header()->session_id;
            int s_id = packet->header()->index;
            uint64_t type = packet->header()->id;
            ring_id_t dest_id = packet->header()->key_id;
            ring_id_t client_key = packet->header()->relay_reply_id;

            this->in_socket_node_instance->handle_external_messaging(false, session_id, s_id, type, dest_id, client_key, packet->payload(), packet->payload_size());

            break;
        }

        case message::external_deaddrop_messaging: {

            DLOG(INFO) << in_socket_node_instance->getid()  << "parsing a external_deaddrop_messaging packet";
            bool is_dest = packet->header()->is_dest;
            int session_id = packet->header()->session_id;
            int seq_id = packet->header()->index;
            uint64_t type = packet->header()->id;
            ring_id_t dest_id = packet->header()->key_id;
            ring_id_t client_key = packet->header()->relay_reply_id;

//            DLOG(INFO) << "packet content: " << target<< " from: " << client_key << " to: " << dest_id;

            if(is_dest){

                if(this->in_socket_node_instance->check_existence(session_id, seq_id)){
                    DLOG(INFO) << "someone in list";

                    std::unique_ptr<AnonymousPacket> pre_packet;
                    msg_info msg = std::make_tuple(session_id, seq_id);
                    pre_packet = std::move(this->in_socket_node_instance->recv_list_deaddrop[msg]);
                    this->in_socket_node_instance->recv_list_deaddrop.erase(msg);

                    ring_id_t pre_client_key = pre_packet->header()->relay_reply_id;

//                    DLOG(INFO) << pre_client_key <<" has been in list, get his content: " << pre_target;
                    //swap & resend
                    uint64_t reply_msg_type = message::external_reply_deaddrop_messaging;

                    this->in_socket_node_instance->handle_external_messaging(false, session_id, seq_id, reply_msg_type, client_key, dest_id, pre_packet->payload(), pre_packet->payload_size());

                    this->in_socket_node_instance->handle_external_messaging(false, session_id, seq_id, reply_msg_type, pre_client_key, dest_id, packet->payload(), packet->payload_size());

                }else{
                    DLOG(INFO) << client_key << "be the first to dead drop";

                    msg_info msg = std::make_tuple(session_id, seq_id);

                    this->in_socket_node_instance->recv_list_deaddrop[msg] = std::move(packet);

                    DLOG(INFO) << "not find swap packet, push, my session id: " <<  session_id << " seq: " << seq_id ;
                    //uint64_t reply_msg_type = message::external_reply_deaddrop_messaging;
                    //this->in_socket_node_instance->handle_external_messaging(false, s_id + 1, reply_msg_type, client_key, dest_id, "Wait for swap", size);
                }


            }else {
                DLOG(INFO) << in_socket_node_instance->getid() << "not dead drop, pass";
                this->in_socket_node_instance->handle_external_messaging(false, session_id, seq_id, type, dest_id, client_key, packet->payload(), packet->payload_size());
            }


            break;
        }

        case message::external_reply_deaddrop_messaging:{
            DLOG(INFO) << in_socket_node_instance->getid()  << "parsing a external_reply_deaddrop_messaging packet";
            bool is_dest = packet->header()->is_dest;
            int session_id = packet->header()->session_id;
            int seq_id = packet->header()->index;
            uint64_t type = packet->header()->id;
            ring_id_t dest_id = packet->header()->key_id;
            ring_id_t client_key = packet->header()->relay_reply_id;

            if(is_dest){
                this->in_socket_node_instance->recv_list_user.push(std::move(packet));
                DLOG(INFO) << "recv queue size: " << this->in_socket_node_instance->recv_list_user.size();
//                DLOG(INFO) << in_socket_node_instance->getid()  << "complete a deaddrop msging. session id: " <<  session_id << " seq: " << seq_id <<
//                " msg type: " << type << " from: " << client_key << " package: " << target;

            }else{
                this->in_socket_node_instance->handle_external_messaging(false,session_id, seq_id, type, dest_id, client_key, packet->payload(), packet->payload_size());
            }
            break;
        }

        //TODO: dummy msg should be sent as usual
        case message::dummy_message: {
            //LOG(INFO) << "get dummy message, dropping it";
            break;
        }

        default:
            LOG(ERROR) << "cannot process internal message type " << id;
    }

}

host_port_t RandomSocket::make_host(const std::string host, int port, uint64_t id) {
    host_port_t t = {.host = host, .port = port, .relay_id = id};
    t.sock.sin_port = htons(port);
    t.sock.sin_family = AF_INET;
    inet_aton(host.c_str(), &t.sock.sin_addr);
    return t;
}

void RandomSocket::safe_send(std::unique_ptr<AnonymousPacket> packet, const char* _addr, int dest_port) {
    pool->send(std::move(packet));
}
