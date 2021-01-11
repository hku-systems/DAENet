//
// Created by jianyu on 5/21/19.
//

#ifndef ANONYMOUSP2P_DAESOCKET_HPP
#define ANONYMOUSP2P_DAESOCKET_HPP


#include <string>
#include <interface/interface.hpp>
#include "../interface.hpp"
#include "Sharp.h"
#include "p2p/DAEService.hpp"
#include "utils/RandomGenerator.hpp"

using namespace anonymous;

typedef struct {

    int session_ID;

    int window_size;

    int64_t shared_secret;

    std::string timeout;

    std::string transmission_rate;

}AppConfig;

class ConfigFile{
public:
    AppConfig config = {
            .session_ID = 1,
            .window_size = 3,
            .shared_secret = 10,
            .timeout = "3000",
            .transmission_rate = "1"
    };

    ConfigFile(int s_id, std::string rate, int64_t seed){
        config.session_ID = s_id;
        config.transmission_rate = rate;
        config.shared_secret = seed;
    }

    ConfigFile(){}
};



class DAESocket: public anonymous::AnonymousSocket {
public:
    int sockfd;
    bool is_sender;

    DAEService* DAERelayService;

    RandomGenerator random;

    uint64_t service_type;

    ring_id_t my_key;

    ring_id_t max_id;

    ring_id_t connector;

    int session_ID;

    std::string transmission_rate;

    int seq;

    int window_size;

    int64_t seed;

    bool is_active = true;

    explicit DAESocket(DAEService* dae_socket, uint64_t _type, ring_id_t client_key, ConfigFile* config):AnonymousSocket(1){
        this->DAERelayService = dae_socket;
        this->service_type = _type;
        this->my_key = client_key;
        this->session_ID = config->config.session_ID;
        this->transmission_rate = config->config.transmission_rate;
        this->seed = config->config.shared_secret;
        this->seq = 0;
        max_id = (1 << dae_socket->m);
        random.srand(seed);
    }


    // send a buf with the size
    void send(char* buf, int size);

    // recv the buf with a maximum len of size
    int recv(char* buf, int size);

    //time out
    int recv(char *buf, int size, int mill);

    void close();

    inline bool is_valid() { return sockfd != -1; }
};


#endif //ANONYMOUSP2P_DAESOCKET_HPP
