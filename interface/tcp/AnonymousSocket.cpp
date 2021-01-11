//
// Created by jianyu on 5/21/19.
//

#include <glog/logging.h>
#include "interface/interface.hpp"
#include "../c_network.h"
#include <iostream>
#include <vector>
#include <fstream>
#include <sodium.h>
#include <cstring>

typedef struct {
    std::string ip;
    int port;
} tcp_host_t;

const char* hardcode_key = "abcdef";

class RelayMetadata {
public:
    int relay_idx = -1;
    std::vector<tcp_host_t> relays;
    unsigned char key[crypto_aead_aes256gcm_KEYBYTES];
    RelayMetadata() {
        std::memcpy(key, hardcode_key, strlen(hardcode_key));
    }
};

class TcpSocket: public anonymous::AnonymousSocket {
public:

    unsigned char shared_buffer[MAX_MTU * 2];

    uint32_t used_buffer_size;

    uint32_t* header;

    explicit TcpSocket(int fd): anonymous::AnonymousSocket(fd) {
        used_buffer_size = 0;
        header = (uint32_t*)shared_buffer;
    }

    void send(char *buf, int size) override {
        unsigned char full_buf[MAX_MTU];
        uint32_t *header = (uint32_t*)full_buf;
        unsigned char* buffer = full_buf + sizeof(uint32_t);
        randombytes_buf(buffer, crypto_aead_aes256gcm_NPUBBYTES);
        std::memcpy(buffer + crypto_aead_aes256gcm_NPUBBYTES, buf, size);
        unsigned long long l;
        crypto_aead_aes256gcm_encrypt((unsigned char*)buffer + crypto_aead_aes256gcm_NPUBBYTES, &l,
                                      (unsigned char*)buf, size,
                                      NULL, 0, NULL, (unsigned char*)buffer, anonymous::AnonymousRelay::current->metadata->key);
        l += crypto_aead_aes256gcm_NPUBBYTES;

        uint32_t s = l + sizeof(uint32_t);
        *header = s;
        c_network::send(sockfd, full_buf, s, 0);
    }

    int recv(char *buf, int size) override {

        // not enough package is received
        int total_size = used_buffer_size;
        int len;
        uint32_t expected_size = (total_size == 0)? size : *header;
        // LOG(INFO) << "expected " << expected_size << " " << used_buffer_size;
        while (total_size < expected_size) {
            len = c_network::recv(sockfd, shared_buffer + total_size, expected_size, 0);
            if (len <= 0) {
                LOG(ERROR) << "connection reset";
                return -1;
            }
            total_size += len;
            expected_size = *header;
            // LOG(INFO) << "expected " << expected_size << " " << used_buffer_size;
        }

        unsigned char *buffer = (unsigned char*)shared_buffer + sizeof(uint32_t);

        // extra bytes
        used_buffer_size = total_size - expected_size;
        total_size = expected_size - sizeof(uint32_t);

        unsigned long long l;
        if (total_size < (crypto_aead_aes256gcm_NPUBBYTES + crypto_aead_aes256gcm_ABYTES) ||
            crypto_aead_aes256gcm_decrypt((unsigned char *) buf, &l,
                                          NULL,
                                          (unsigned char *) buffer + crypto_aead_aes256gcm_NPUBBYTES,
                                          total_size - crypto_aead_aes256gcm_NPUBBYTES,
                                          NULL, 0,
                                          (unsigned char *) buffer, anonymous::AnonymousRelay::current->metadata->key) != 0) {
            LOG(ERROR) << "decryption error " << total_size;
            return -1;
        }

        if (used_buffer_size) {
            std::memcpy(shared_buffer, shared_buffer + expected_size, used_buffer_size);
        }

        return l;
    }

    void close() override {
        return c_network::close(sockfd);
    }
};

anonymous::AnonymousRelay* anonymous::AnonymousRelay::current = NULL;

std::string anonymous::AnonymousRelay::initialize(std::string ip, int port, std::string meta) {

    metadata = new RelayMetadata();

    std::string meta_file = "tcp-" + meta;

    std::fstream config(meta_file);

    int idx = 0, meta_port;
    std::string meta_ip;
    while (config >> meta_ip >> meta_port) {
        if (ip == meta_ip && port == meta_port) {
            metadata->relay_idx = idx;
        }
        tcp_host_t config_entry = {
                .ip = meta_ip,
                .port = meta_port
        };
        metadata->relays.push_back(config_entry);
        idx++;
    }

    if (metadata->relay_idx == -1) {
        LOG(ERROR) << "cannot find server config";
        return "";
    }

    sockfd = c_network::socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    if (c_network::bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        LOG(ERROR) << "error binding addr ";
        return "";
    }

    if (c_network::listen(sockfd, 5) < 0) {
        LOG(ERROR) << "error listening addr ";
        return "";
    }

    LOG(INFO) << "init with id " << std::to_string(metadata->relay_idx);

    current = this;

    return std::to_string(metadata->relay_idx);
}

std::unique_ptr<anonymous::AnonymousSocket> anonymous::AnonymousRelay::connect(std::string id) {
    int idx = std::stoi(id);

    if (idx >= metadata->relays.size()) {
        LOG(ERROR) << "destination not exists";
        return std::make_unique<TcpSocket>(-1);
    }

    struct sockaddr_in server_addr;

    int sock = c_network::socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(metadata->relays[idx].port);
    if (inet_pton(AF_INET, metadata->relays[idx].ip.c_str(), &(server_addr.sin_addr)) < 0) {
        LOG(ERROR) << "error during ip conversion";
    }
    if (c_network::connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        LOG(ERROR) << "error during connection ";
    }

    return std::make_unique<TcpSocket>(sock);
}

std::unique_ptr<anonymous::AnonymousSocket> anonymous::AnonymousRelay::accept() {
    struct sockaddr_in server_addr;
    socklen_t len;
    int accept_fd;
    if ((accept_fd = c_network::accept(sockfd, (struct sockaddr*)&(server_addr), &len)) < 0) {
        LOG(ERROR) << "error during accepting connection";
        return std::make_unique<TcpSocket>(-1);
    }
    return std::make_unique<TcpSocket>(accept_fd);
}

std::string anonymous::AnonymousRelay::id_to_key(ring_id_t id) {
    return std::to_string(id);
}

ring_id_t anonymous::AnonymousRelay::key_to_id(std::string key) {
    return std::stoi(key);
}

ring_id_t anonymous::AnonymousRelay::id_range() {
    return current->metadata->relays.size();
}