//
// Created by jianyu on 2/21/19.
//

#ifndef ANONYMOUSP2P_SOCKETHEADER_HPP
#define ANONYMOUSP2P_SOCKETHEADER_HPP

#include <string>
#include <netinet/in.h>

typedef struct {
    std::string host;
    int         port;
    uint64_t    relay_id;
    struct sockaddr_in sock;
} host_port_t;

bool operator<(const host_port_t& a, const host_port_t &b);

bool operator>(const host_port_t& a, const host_port_t &b);

bool operator<=(const host_port_t& a, const host_port_t &b);

bool operator>=(const host_port_t& a, const host_port_t &b);

#endif //ANONYMOUSP2P_SOCKETHEADER_HPP
