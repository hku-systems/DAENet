//
// Created by Maxxie Jiang on 6/6/2019.
//

#include "ShufflePool.hpp"
#include "p2p/DAENode.hpp"
#include "utils/ConcurrentHeap.hpp"
#include "relay/RandomSocket.hpp"
#include <glog/logging.h>
#include <utils/EventTask.hpp>
#include <cmath>
#include "message/message.hpp"

void ShufflePool::start() {
    struct timeval tv = {
        .tv_sec = 0,
        .tv_usec = msg_rate
    };
    EventTask::global->add_time_task_lambda(tv, [this](struct timeval *tv){
        send_batch();
    });
}

void ShufflePool::send(std::unique_ptr<AnonymousPacket> packet) {
    for (int i = 0;i < m;i++) {
        if (strcmp(packet->header()->relay_dest_ip, finger_table[i].ip) == 0 &&
            packet->header()->relay_dest_port == finger_table[i].port) {
            package_pool[i].push(std::move(packet));
            return;
        }
    }

    if (strcmp(packet->header()->relay_dest_ip, socket->host.c_str()) == 0 &&
            packet->header()->relay_dest_port == socket->port) {
        socket->DAE_direct_send(std::move(packet), packet->header()->relay_dest_ip, packet->header()->relay_dest_port);
        return;
    }

    LOG(ERROR) << "cannot find destination in the finger table, packet ip: " << packet->header()->relay_dest_ip;
    LOG(ERROR) << "  >> sucessor ip: " << finger_table[0].ip;
} 

void ShufflePool::send_batch() {
    bool should_print = false;
    int cur = -1;
    for (auto &queue: package_pool) {
        cur += 1;
        debug_total_msg += 1;
        auto pkg = queue.pop_with_P(shuffle_P);
        if (pkg == nullptr) {
            // send dummy
            if(send_dummy){
                auto dummy_msg = new AnonymousPacket();
                dummy_msg->header()->id = message::dummy_message;
                dummy_msg->header()->msg_type = true;
                dummy_msg->header()->size = 132;  // TODO
                dummy_msg->header()->relay_dest_id = finger_table[cur].successor;
                strcpy(dummy_msg->header()->relay_dest_ip, finger_table[cur].ip);
                dummy_msg->header()->relay_dest_port = finger_table[cur].port;
                this->socket->in_socket_node_instance->dummy_cnt += 1;
                socket->DAE_direct_send(static_cast<std::unique_ptr<AnonymousPacket>>(std::move(dummy_msg)), dummy_msg->header()->relay_dest_ip, dummy_msg->header()->relay_dest_port);
            }
            continue;
        }
        debug_real_forward_msg += 1;
        char ip[20];
        strcpy(ip, pkg->header()->relay_dest_ip);
        int port = pkg->header()->relay_dest_port;
        this->socket->in_socket_node_instance->app_cnt += 1;
        socket->DAE_direct_send(std::move(pkg), ip, port);
    }

    auto time_now = system_clock::now();
    if (duration<double>(time_now - time_prev).count() >= this->socket->in_socket_node_instance->cnt_interval) {
        should_print = true;
        time_prev = time_now;
    }
    if (should_print && debug) {
        // print the detail
        LOG(INFO) << "(dummy:app:p2p) " << this->socket->in_socket_node_instance->dummy_cnt << " " <<this->socket->in_socket_node_instance->app_cnt << " " << this->socket->in_socket_node_instance->p2p_cnt - this->socket->in_socket_node_instance->dummy_cnt;
        //LOG(INFO) << "real/forward " << debug_real_forward_msg << ", " << debug_total_msg << ", " << (double)(debug_real_forward_msg * 100) / debug_total_msg;
    }

    if (should_print) {
        this->socket->in_socket_node_instance->dummy_cnt = 0;
        this->socket->in_socket_node_instance->app_cnt = 0;
        this->socket->in_socket_node_instance->p2p_cnt = 0;
        debug_real_forward_msg = 0;
        debug_total_msg = 0;
    }

}