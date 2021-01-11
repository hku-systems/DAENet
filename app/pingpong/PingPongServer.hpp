
#ifndef ANONYMOUSP2P_PINGPONGSERVER_HPP
#define ANONYMOUSP2P_PINGPONGSERVER_HPP

#include "interface.hpp"
#include "../service.hpp"

using namespace anonymous;

typedef struct {
    char id[128];
    uint32_t idx;
} ping_t;

class PingPongServer: public AnonymousService {
public:
    std::string key;
    AnonymousRelay relay;
    explicit PingPongServer(const service_t &service): AnonymousService(service) {}
    bool init(std::string);
    void ping(std::string s, int n);
    void start();
};

#endif //ANONYMOUSP2P_PINGPONGSERVER_HPP
