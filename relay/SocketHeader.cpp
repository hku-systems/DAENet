//
// Created by jianyu on 2/23/19.
//

#include "SocketHeader.hpp"

bool operator<(const host_port_t& a, const host_port_t &b) {
    return a.relay_id < b.relay_id;
}

bool operator>(const host_port_t& a, const host_port_t &b) {
    return a.relay_id < b.relay_id;
}

bool operator<=(const host_port_t& a, const host_port_t &b) {
    return a.relay_id <= b.relay_id;
}

bool operator>=(const host_port_t& a, const host_port_t &b) {
    return a.relay_id <= b.relay_id;
}