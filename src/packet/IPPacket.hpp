//
// Created by jianyu on 1/28/19.
//

#ifndef ANONYMOUSP2P_PACKAGE_HPP
#define ANONYMOUSP2P_PACKAGE_HPP

#include <string>
#include "packet.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "packet.h"
#include "packet_checksum.h"
#ifdef __cplusplus
} // closing brace for extern "C"
#endif

class IPPacket {
public:
    struct packet pkg;
public:

    inline const ipv4* ipv4_header() const { return &pkg.hdr.v4.ip_hdr; }

    inline std::string src_ip() const {
        char* ip_name = inet_ntoa(ipv4_header()->src_ip);
        return std::move(std::string(ip_name));
    }

    inline std::string dest_ip() const {
        char* ip_name = inet_ntoa(ipv4_header()->dst_ip);
        return std::move(std::string(ip_name));
    }

    inline uint32_t raw_src_ip() const {
        return ipv4_header()->src_ip.s_addr;
    }

    inline uint32_t raw_dest_ip() const {
        return ipv4_header()->dst_ip.s_addr;
    }

    inline int set_src_ip(const std::string &ip) {
        const char* ip_c = ip.c_str();
        return inet_aton(ip_c, &pkg.hdr.v4.ip_hdr.src_ip);
    }

    inline int set_dst_ip(const std::string &ip) {
        const char* ip_c = ip.c_str();
        return inet_aton(ip_c, &pkg.hdr.v4.ip_hdr.dst_ip);
    }

    inline void set_raw_src(uint32_t ip) {
        pkg.hdr.v4.ip_hdr.src_ip.s_addr = ip;
    }

    inline void set_raw_dest(uint32_t ip) {
        pkg.hdr.v4.ip_hdr.dst_ip.s_addr = ip;
    }

    inline void checksum() {
        checksum_packet(&pkg);
    }

    struct icmpv4* icmpv4() { return (struct icmpv4*)((unsigned char*)this + ipv4_header_len(&pkg.hdr.v4.ip_hdr)); }

    inline bool is_ip() { return ipv4_header()->version == ip_ipv4; }
    inline bool is_support_layer4() { return ipv4_header()->protocol == IPPROTO_ICMP
                                            || ipv4_header()->protocol == IPPROTO_TCP
                                            || ipv4_header()->protocol == IPPROTO_ICMP; }
};


#endif //ANONYMOUSP2P_PACKAGE_HPP
