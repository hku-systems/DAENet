//
// Created by jianyu on 6/7/19.
//

#ifndef ANONYMOUSAPPLICATION_BUFFERSEND_HPP
#define ANONYMOUSAPPLICATION_BUFFERSEND_HPP

#include "../service.hpp"
#include <interface.hpp>
using namespace anonymous;

class BufferSend: public AnonymousService {
public:
    std::string key;
    AnonymousRelay relay;
    explicit BufferSend(const service_t &service): AnonymousService(service) {}
    bool init(std::string);
    void fetch(std::string s, int n);
    void start();
};


#endif //ANONYMOUSAPPLICATION_BUFFERSEND_HPP
