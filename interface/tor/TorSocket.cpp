//
// Created by jianyu on 5/27/19.
//

#include <fstream>
#include <relay/RelayConfig.hpp>
#include <cstring>
#include "interface/interface.hpp"
#include "../c_network.h"

using namespace anonymous;

#define NUM_RELAYS metadata->num_layers

typedef struct {
    std::string ip;
    int port;
    std::string key;
    in_addr_t ip_raw;
    char raw_key[crypto_aead_aes256gcm_KEYBYTES];
} tor_peer_t;

typedef struct {
    uint8_t is_dest;
    in_addr_t ip;
    uint16_t port;
} tor_message_t;

class RelayMetadata {
public:
    std::vector<tor_peer_t> all_relays;
    std::string tor_key;
    int server_idx;
    int num_layers;
    unsigned char raw_key[crypto_aead_aes256gcm_KEYBYTES];
};

inline static char* recv_with_header_size(char* shared_buffer, int &used_buffer_size, int sockfd, int size, uint32_t &total_size) {
    uint32_t *header = (uint32_t*)shared_buffer;
    total_size = used_buffer_size;
    int len;
    uint32_t expected_size = (total_size == 0)? size : *header;
    // LOG(INFO) << "expected " << expected_size << " " << used_buffer_size;
    while (total_size < expected_size) {
        len = c_network::recv(sockfd, shared_buffer + total_size, expected_size, 0);
        if (len <= 0) {
            LOG(INFO) << "connection reset";
            return NULL;
        }
        total_size += len;
        expected_size = *header;
        // LOG(INFO) << "expected " << expected_size << " " << used_buffer_size;
    }

    char *buffer = (char*)shared_buffer + sizeof(uint32_t);

    // extra bytes
    used_buffer_size = total_size - expected_size;
    total_size = expected_size - sizeof(uint32_t);
    return buffer;
}

// format
// [tor_msg + [payload]]

// decrypt/encrypt the package and send
// used by the forward relay
static int recv_crypto_send(int from_socket, int to_socket, bool encrypt, const unsigned char* raw_key, char *shared_buffer, int &shared_size) {
    char *from_buffer;
    char output_buffer[MAX_MTU];
    char *to_buffer = output_buffer + sizeof(uint32_t);
    uint32_t total_size;

    if ((from_buffer = recv_with_header_size(shared_buffer, shared_size, from_socket, MAX_MTU, total_size)) == NULL) {
        return -1;
    }

    unsigned long long l;
    if (encrypt) {
        randombytes_buf(to_buffer, crypto_aead_aes256gcm_NPUBBYTES);
        crypto_aead_aes256gcm_encrypt((unsigned char*)to_buffer + crypto_aead_aes256gcm_NPUBBYTES, &l,
                                      (unsigned char*)from_buffer, total_size,
                                      NULL, 0, NULL, (unsigned char*)to_buffer, raw_key);
        l += crypto_aead_aes256gcm_NPUBBYTES;
    } else {
        if (total_size < (crypto_aead_aes256gcm_NPUBBYTES + crypto_aead_aes256gcm_ABYTES) ||
            crypto_aead_aes256gcm_decrypt((unsigned char *) to_buffer, &l,
                                          NULL,
                                          (unsigned char *) from_buffer + crypto_aead_aes256gcm_NPUBBYTES,
                                          total_size - crypto_aead_aes256gcm_NPUBBYTES,
                                          NULL, 0,
                                          (unsigned char *) from_buffer, raw_key) != 0) {
            LOG(ERROR) << "decryption error " << total_size;
            return -1;
        }
    }

    if (shared_size) {
        std::memcpy(shared_buffer, shared_buffer + total_size + sizeof(uint32_t), shared_size);
    }

    uint32_t send_size = l + sizeof(uint32_t);
    *((uint32_t*)output_buffer) = send_size;
    c_network::send(to_socket, output_buffer, send_size, 0);

    return l;
}

static int recv_decrypt_onion(int sockfd, char *buf, const unsigned char* raw_key, char *shared_buffer, int &shared_size, int layer) {
    char buffer[MAX_MTU];
    char *recv_buffer;
    uint32_t expected_size;

    if ((recv_buffer = recv_with_header_size(shared_buffer, shared_size, sockfd, MAX_MTU, expected_size)) == NULL) {
        return -1;
    }

    std::memcpy(buffer, recv_buffer, expected_size);

    if (shared_size) {
        std::memcpy(shared_buffer, shared_buffer + expected_size + sizeof(uint32_t), shared_size);
    }

//    LOG(INFO) << "get size " << expected_size << " " << total_size;

    unsigned long long l = -1;
    for (int i = 0;i < layer + 1;i++) {
        // onion decrypt
        if (expected_size < (crypto_aead_aes256gcm_NPUBBYTES + crypto_aead_aes256gcm_ABYTES) ||
            crypto_aead_aes256gcm_decrypt((unsigned char *) buf, &l,
                                          NULL,
                                          (unsigned char *) buffer + crypto_aead_aes256gcm_NPUBBYTES,
                                          expected_size - crypto_aead_aes256gcm_NPUBBYTES,
                                          NULL, 0,
                                          (unsigned char *) buffer, raw_key) != 0) {
            LOG(ERROR) << "decryption error " << expected_size;
            return -1;
        }
        std::memcpy(buffer, buf, l);
        expected_size = l;
    }
    return l;
}

static int raw_connect(in_addr_t ip, int port) {
    struct sockaddr_in server_addr;
    int sock = c_network::socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = ip;

    if (c_network::connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        LOG(INFO) << "error during connection ";
        return -1;
    }
    return sock;
}

class TorSocket: public AnonymousSocket {
public:

    AnonymousRelay *relay;

    explicit TorSocket(int fd, AnonymousRelay* r, bool sender = false): AnonymousSocket(fd), relay(r), is_sender(sender) {
        relays = new tor_peer_t[relay->metadata->num_layers + 1];
    }

    ~TorSocket() {
        delete[] relays;
    }

    bool is_sender;

    tor_peer_t *relays;

    void generate_circuits(char *buf, int &size, bool header, int layer) {
        char buffer[MAX_MTU];
        auto *msg_header = reinterpret_cast<tor_message_t*>(buffer);

        unsigned long long len = size;
        int plaintext_len = len;

        // destionation encryption
        randombytes_buf(buffer, crypto_aead_aes256gcm_NPUBBYTES);
        crypto_aead_aes256gcm_encrypt((unsigned char*)buffer + crypto_aead_aes256gcm_NPUBBYTES, &len,
                                      (unsigned char*)buf, plaintext_len,
                                      NULL, 0, NULL, (unsigned char*)buffer, relay->metadata->raw_key);
        len += crypto_aead_aes256gcm_NPUBBYTES;

        std::memcpy(buf, buffer, len);

        // start the onion encryption
        for (int i = layer - 1;i >= 0;i--) {
            int offset = 0;
            if (header) {
                offset = sizeof(tor_message_t);
            }
            std::memcpy(buffer + offset, buf, len);
            len += offset;
            plaintext_len = len;

            if (header) {
                msg_header->is_dest = 0;
                msg_header->ip = relays[i + 1].ip_raw;
                msg_header->port = relays[i + 1].port;
            }

            randombytes_buf(buf, crypto_aead_aes256gcm_NPUBBYTES);
            crypto_aead_aes256gcm_encrypt((unsigned char*)buf + crypto_aead_aes256gcm_NPUBBYTES, &len,
                                          (unsigned char*)buffer, plaintext_len,
                                          NULL, 0, NULL, (unsigned char*)buf, relay->metadata->raw_key);
            len += crypto_aead_aes256gcm_NPUBBYTES;
        }
        size = len;
    }

    char shared_buffer[MAX_MTU];
    int shared_buffer_size = 0;

    void send(char *from_buf, int size) override {
        char buff[MAX_MTU];
        char *buf = buff + sizeof(uint32_t);
        if (this->is_sender) {
            // generate circuits
            if (size == -1) {
                size = sizeof(tor_message_t);
                std::memcpy(buf, from_buf, size);
                generate_circuits(buf, size, true, relay->metadata->num_layers);
            } else {
                std::memcpy(buf, from_buf, size);
                generate_circuits(buf, size, false, relay->metadata->num_layers);
            }
        } else {
            std::memcpy(buf, from_buf, size);
            // the target will not send the header
            generate_circuits(buf, size, false, 0);
        }
        uint32_t s = size + sizeof(uint32_t);
        *((uint32_t*)buff) = s;
        // then send the content
        c_network::send(sockfd, buff, s, 0);
//        LOG(INFO) << "send " << size;
    }

    int recv(char *buf, int size) override {
        if (this->is_sender) {
            return recv_decrypt_onion(sockfd, buf, relay->metadata->raw_key, shared_buffer, shared_buffer_size, relay->metadata->num_layers);
        } else {
            return recv_decrypt_onion(sockfd, buf, relay->metadata->raw_key, shared_buffer, shared_buffer_size, 0);
        }
    }

    void close() {
        return c_network::close(sockfd);
    }
};

AnonymousRelay* AnonymousRelay::current = NULL;

std::string AnonymousRelay::initialize(std::string ip, int port, std::string meta) {

    metadata = new RelayMetadata();

    meta = "tor-" + meta;

    // read the json file
    std::ifstream config_stream(meta);
    std::string relay_ip, key;
    int relay_port;

    tor_peer_t pair;

    bool find_me = false;
    std::memset(metadata->raw_key, 0, crypto_aead_aes256gcm_KEYBYTES);

    int cur_idx = 0;
    while (config_stream >> relay_ip >> relay_port) {
        pair.ip = relay_ip;
        pair.port = relay_port;
        pair.key = key;

        if (relay_ip == "layer") {
            metadata->num_layers = relay_port;
            continue;
        }

        config_stream >> key;

        if (relay_ip == ip && relay_port == port) {
            metadata->tor_key = key;
            // TODO: check len
            if (metadata->tor_key.length() >= crypto_aead_aes256gcm_KEYBYTES) {
                LOG(ERROR) << "key is too long";
                return "";
            }
            std::strcpy((char*)metadata->raw_key, metadata->tor_key.c_str());
            find_me = true;
            metadata->server_idx = cur_idx;
        }
        // LOG(INFO) << "Config: " << relay_ip << ":" << relay_port << " key:" << key;
        metadata->all_relays.push_back(pair);
        cur_idx += 1;
    }
    if (!find_me) {
        LOG(ERROR) << "cannot find ip in the configuration file";
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

    for (tor_peer_t &peer: metadata->all_relays) {
        if (inet_pton(AF_INET, peer.ip.c_str(), &(server_addr.sin_addr)) < 0) {
            LOG(ERROR) << "error during ip conversion";
        }
        peer.ip_raw = server_addr.sin_addr.s_addr;
    }

    std::string id = std::to_string(metadata->server_idx);

    DLOG(INFO) << "init with id " << id;

    current = this;

    return id;
}

std::unique_ptr<AnonymousSocket> AnonymousRelay::connect(std::string id) {

    auto socket = std::make_unique<TorSocket>(-1, this, true);

    int idi = std::stoi(id);
    if (idi >= metadata->all_relays.size()) {
        LOG(ERROR) << "error finding the destionation " << id;
        return socket;
    }

    socket->relays[NUM_RELAYS] = metadata->all_relays[idi];

    // find three circuits
    int relay_idx;
    for (int i = 0;i < NUM_RELAYS;i++) {
        relay_idx = randombytes_random() % AnonymousRelay::id_range();
        while (relay_idx == metadata->server_idx || relay_idx == idi) {
            relay_idx = randombytes_random() % AnonymousRelay::id_range();
        }
        socket->relays[i] = metadata->all_relays[relay_idx];
        LOG(INFO) << "relay " << relay_idx << " is selected. " << "ip: " << socket->relays[i].ip << ":" << socket->relays[i].port;
    }

    struct sockaddr_in server_addr;
    int sock = c_network::socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(socket->relays[0].port);
    server_addr.sin_addr.s_addr = socket->relays[0].ip_raw;

    if (c_network::connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        LOG(ERROR) << "error during connection ";
    }

    socket->sockfd = sock;

    // send a empty msg to creat circuits
    char buf[MAX_MTU];
    auto msg = reinterpret_cast<tor_message_t*>(buf);
    msg->is_dest = 1;
    socket->send(buf, -1);
    return socket;
}


std::unique_ptr<anonymous::AnonymousSocket> anonymous::AnonymousRelay::accept() {
    struct sockaddr_in server_addr;
    socklen_t len;
    int accept_fd;

    while (true) {
        if ((accept_fd = c_network::accept(sockfd, (struct sockaddr*)&(server_addr), &len)) < 0) {
            LOG(ERROR) << "error during accepting connection";
            return std::make_unique<TorSocket>(-1, this);
        }
        auto socket = std::make_unique<TorSocket>(accept_fd, this);
        char buf[MAX_MTU];
        auto *msg = reinterpret_cast<tor_message_t*>(buf);
        int size = socket->recv((char*)buf, sizeof(tor_message_t));
        if (msg->is_dest == 1) {
            LOG(INFO) << "Real Request ";
            return socket;
        } else {
            // handle forward
            tor_message_t msg_cpy = *msg;
            int raw_socket = socket->sockfd;
            std::thread([socket = std::move(socket), msg_cpy, raw_socket, size, buf, this](){
                LOG(INFO) << "Forward Request ";
                int next_hop_socket = raw_connect(msg_cpy.ip, msg_cpy.port);

                if (next_hop_socket == -1) {
                    socket->close();
                    LOG(ERROR) << "error during forward connection";
                    return;
                }

                uint32_t s = size - sizeof(tor_message_t);

                unsigned char tmp_buf[s + sizeof(uint32_t)];
                *((uint32_t*)tmp_buf) = s + sizeof(uint32_t);
                std::memcpy(tmp_buf + sizeof(uint32_t), buf + sizeof(tor_message_t), s);

                c_network::send(next_hop_socket, tmp_buf, s + sizeof(uint32_t), 0);

                // create a thread to forward the result from next_hop
                std::thread([this, raw_socket, next_hop_socket](){
                    char shared_buffer[MAX_MTU];
                    int shared_buffer_size = 0;
                    while (true) {
                        if (recv_crypto_send(next_hop_socket, raw_socket, true, metadata->raw_key, shared_buffer, shared_buffer_size) < 0) {
                            c_network::close(raw_socket);
                            LOG(INFO) << "circuit closes, close socket from the previous hop";
                            break;
                        }
                    }
                }).detach();

                char shared_buffer[MAX_MTU];
                int shared_buffer_size = 0;
                while (true) {
                    // recv decrypt and send
                    if (recv_crypto_send(raw_socket, next_hop_socket, false, metadata->raw_key, shared_buffer, shared_buffer_size) < 0) {
                        c_network::close(next_hop_socket);
                        LOG(INFO) << "circuit closes, close socket to next hop";
                        break;
                    }
                }
            }).detach();
            continue;
        }
    }
}

std::string anonymous::AnonymousRelay::id_to_key(ring_id_t id) {
    return std::to_string(id);
}

ring_id_t anonymous::AnonymousRelay::key_to_id(std::string key) {
    return std::stoi(key);
}

ring_id_t anonymous::AnonymousRelay::id_range() {
    return current->metadata->all_relays.size();
}