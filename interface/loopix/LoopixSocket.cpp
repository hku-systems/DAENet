
#include "interface/interface.hpp"
#include "../c_network.h"
#include <glog/logging.h>
#include <map>
#include <fstream>
#include <cstring>
#include <utils/ConcurrentBlockQueue.hpp>

// format [is_connection, src(2), dest(2), sock_idx]
typedef struct {
    unsigned char buf[MAX_MTU];
    int size;
} loopix_packet_t;

class RelayMetadata {
public:
    int relay_idx;
    int relay_num;
    int loopix_client_port = 0;
    int loopix_backend_port = 0;
    int loopix_local_port = 0;

    struct sockaddr_in local_addr;
    struct sockaddr_in loopix_backend_addr;

    // lock and conditional variable
    std::mutex mutex;

    ConcurrentBlockQueue<std::unique_ptr<loopix_packet_t> > accept_queue;
    std::map<int, ConcurrentBlockQueue<std::unique_ptr<loopix_packet_t>>*> socket_queue;

    ConcurrentBlockQueue<std::unique_ptr<loopix_packet_t>>* get_queue(int idx) {
        std::unique_lock<std::mutex> lk(mutex);
        return socket_queue[idx];
    }

    ConcurrentBlockQueue<std::unique_ptr<loopix_packet_t>>* register_queue(int idx) {
        std::unique_lock<std::mutex> lk(mutex);
        return socket_queue[idx] = new ConcurrentBlockQueue<std::unique_ptr<loopix_packet_t>>();
    }

    void deregister_queue(int idx) {
        socket_queue[idx] = nullptr;
    }
};

enum {
    connection_bit = 0,
    src_high_bit = 1,
    src_low_bit = 2,
    dest_high_bit = 3,
    dest_low_bit = 4,
    header_len
};

anonymous::AnonymousRelay* anonymous::AnonymousRelay::current = NULL;

class LoopixSocket: public anonymous::AnonymousSocket {
public:

    int loopix_dest_idx;
    anonymous::AnonymousRelay *relay;
    ConcurrentBlockQueue<std::unique_ptr<loopix_packet_t>> *queue;
    bool is_active;
    explicit LoopixSocket(int fd, int dest, anonymous::AnonymousRelay *r): anonymous::AnonymousSocket(fd), loopix_dest_idx(dest), relay(r) {
        queue = relay->metadata->register_queue(dest);
        is_active = true;
    }
    // [format: from_addr, to_addr, payload]
    void send(char *buf, int size) override {
        unsigned char buff[MAX_MTU];
        int loopix_dest_idx_high = loopix_dest_idx >> 8;
        int loopix_dest_idx_low  = loopix_dest_idx % (256);

        int loopix_src_idx_high = relay->metadata->relay_idx >> 8;
        int loopix_src_idx_low = relay->metadata->relay_idx % 256;

        std::memcpy(buff + header_len, buf, size);

        buff[connection_bit] = 0;

        buff[src_high_bit] = loopix_src_idx_high;
        buff[src_low_bit] = loopix_src_idx_low;
        buff[dest_high_bit] = loopix_dest_idx_high;
        buff[dest_low_bit] = loopix_dest_idx_low;

        // LOG(INFO) << "send to backend " << loopix_dest_idx_high << " " << loopix_dest_idx_low;
        c_network::sendto(sockfd, buff, size + header_len, 0, (const sockaddr*) &relay->metadata->loopix_backend_addr, sizeof(relay->metadata->loopix_backend_addr));
    }

    int recv(char *buf, int size) override {
        auto packet = queue->pop_front();
        while (packet == nullptr) {
            packet = queue->pop_front();
        }
        if (!is_active) { return -1; }
        std::memcpy(buf, packet->buf + header_len, packet->size - header_len);
        return packet->size - header_len;
    }

    int recv(char *buf, int size, int mill) override {
        bool is_timeout = false;
        auto packet = queue->pop_front(mill, is_timeout);
        if (is_timeout) return 0;
        while (packet == nullptr) {
            is_timeout = false;
            packet = queue->pop_front(mill, is_timeout);
            if (is_timeout) return 0;
        }
        if (!is_active) { return -1; }
        std::memcpy(buf, packet->buf + header_len, packet->size - header_len);
        return packet->size - header_len;
    }

    void close() override {
        auto pkg = std::make_unique<loopix_packet_t>();
        queue->push(std::move(pkg));
        is_active = false;
    }
};

std::string anonymous::AnonymousRelay::initialize(std::string ip, int port, std::string meta) {

    metadata = new RelayMetadata();

    sockfd = c_network::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    meta = "loopix-" + meta;
    std::ifstream config_stream(meta);
    std::string relay_ip;
    int relay_port;

    int cur_idx = 0;
    while (config_stream >> relay_ip >> relay_port) {
        if (relay_ip == ip && relay_port == port) {
            metadata->relay_idx = cur_idx;
        }
        // LOG(INFO) << "Config: " << relay_ip << ":" << relay_port;
        cur_idx += 1;
    }

    metadata->relay_num = cur_idx;

    metadata->loopix_backend_port = port + 1000;
    metadata->loopix_local_port = port + 1001;
    metadata->loopix_client_port = port;

    memset(&metadata->local_addr, 0, sizeof(metadata->local_addr));
    memset(&metadata->loopix_backend_addr, 0, sizeof(metadata->loopix_backend_addr));

    metadata->local_addr.sin_family = AF_INET;
    metadata->local_addr.sin_port = htons(metadata->loopix_local_port);
    metadata->local_addr.sin_addr.s_addr = INADDR_LOOPBACK;

    metadata->loopix_backend_addr.sin_family = AF_INET;
    metadata->loopix_backend_addr.sin_port = htons(metadata->loopix_backend_port);
    metadata->loopix_backend_addr.sin_addr.s_addr = INADDR_LOOPBACK;

    if (inet_pton(AF_INET, "127.0.0.1", &(metadata->local_addr.sin_addr)) < 0) {
        LOG(ERROR) << "error during ip conversion";
    }

    if (inet_pton(AF_INET, "127.0.0.1", &(metadata->loopix_backend_addr.sin_addr)) < 0) {
        LOG(ERROR) << "error during ip conversion";
    }

    if (c_network::bind(sockfd, (const struct sockaddr*) &metadata->local_addr, sizeof (metadata->local_addr)) < 0) {
        LOG(ERROR) << "bind address " << metadata->loopix_local_port << " fail ";
        return "";
    }

    // message handler thread
    std::thread([this]{
        struct sockaddr_in from_addr;
        socklen_t len;
        while (true) {
            auto packet = std::make_unique<loopix_packet_t>();
            int size = c_network::recvfrom(sockfd, packet->buf, MAX_MTU, 0, (struct sockaddr*) &from_addr, &len);
            if (size <= 0) {
                LOG(ERROR) << "connection close" << size;
                break;
            }
            packet->size = size;

            int src = (packet->buf[src_high_bit] << 8) + packet->buf[src_low_bit];
                
            // new connection
            if (packet->buf[0] == 1) {
                auto queue = metadata->get_queue(src);
                if (queue == nullptr) {
                    metadata->accept_queue.push(std::move(packet));
                    LOG(INFO) << "new connection " << src << ", current: " << metadata->relay_idx;
                } // ignore the connection if exists
            } else {
                auto queue = metadata->get_queue(src);
                if (queue == nullptr) {
                    metadata->accept_queue.push(std::move(packet));
                    LOG(ERROR) << "connection not exists " << src << ", current: " << metadata->relay_idx;
                } else {
                    queue->push(std::move(packet));
                }
            }
            // LOG(INFO) << "receive from backend";
        }

    }).detach();

    // sendto the loopix socket

    std::string id = std::to_string(metadata->relay_idx);

    DLOG(INFO) << "init with id " << id;

    current = this;

    return id;
}

std::unique_ptr<anonymous::AnonymousSocket> anonymous::AnonymousRelay::connect(std::string id) {
    int loopix_dest_idx = std::stoi(id);

    int loopix_dest_idx_high = loopix_dest_idx >> 8;
    int loopix_dest_idx_low  = loopix_dest_idx % (256);

    int loopix_src_idx_high = metadata->relay_idx >> 8;
    int loopix_src_idx_low = metadata->relay_idx % 256;

    char buff[header_len];
    buff[connection_bit] = 1;

    buff[src_high_bit] = loopix_src_idx_high;
    buff[src_low_bit] = loopix_src_idx_low;
    buff[dest_high_bit] = loopix_dest_idx_high;
    buff[dest_low_bit] = loopix_dest_idx_low;

    int pv = (buff[src_high_bit] << 8) + buff[src_low_bit];
    if (pv != metadata->relay_idx) {
        LOG(ERROR) << "error in serializing " << pv << " " << metadata->relay_idx;
    }

    c_network::sendto(sockfd, buff, header_len, 0, (const sockaddr*) &metadata->loopix_backend_addr, sizeof(metadata->loopix_backend_addr));
    c_network::sendto(sockfd, buff, header_len, 0, (const sockaddr*) &metadata->loopix_backend_addr, sizeof(metadata->loopix_backend_addr));
    sleep(2);
    return std::make_unique<LoopixSocket>(sockfd, loopix_dest_idx, this);
}

std::unique_ptr<anonymous::AnonymousSocket> anonymous::AnonymousRelay::accept() {
    auto packet = metadata->accept_queue.pop_front();
    while (packet == nullptr) {
        packet = metadata->accept_queue.pop_front();
    }
    int src = (packet->buf[src_high_bit] << 8) + (packet->buf[src_low_bit]);
    return std::make_unique<LoopixSocket>(sockfd, src, this);
}

std::string anonymous::AnonymousRelay::id_to_key(ring_id_t id) {
    return std::to_string(id);
}

ring_id_t anonymous::AnonymousRelay::key_to_id(std::string key) {
    return std::stoi(key);
}

ring_id_t anonymous::AnonymousRelay::id_range() {
    return current->metadata->relay_num;
}