//
// Created by Maxxie Jiang on 25/3/2019.
//

#ifndef ANONYMOUSP2P_FILEHEADER_HPP
#define ANONYMOUSP2P_FILEHEADER_HPP

#include "AnonymousP2p.hpp"
#include <iostream>
#include "interface.hpp"

namespace message {

    typedef struct {
        uint64_t h;
        uint64_t l;
    } uint128_t;

    bool operator<(const uint128_t& left, const uint128_t& right);

    inline std::string hash(const uint128_t& md5) {
        uint64_t l = md5.l;
        l = l % anonymous::AnonymousRelay::current->id_range();
        return anonymous::AnonymousRelay::current->id_to_key(l);
    }

    void set_md5_from_buf(const uint128_t& md5, const char* buf);

    std::ostream& operator<<(std::ostream&, const uint128_t& md5);
    std::istream& operator>>(std::istream&, uint128_t&);
    enum FileMessageType {
        register_file, register_file_reply, fetch_file_loc, fetch_file_loc_reply,
        fetch_file, file_content
    };

    typedef struct {
        FileMessageType message_type;
        uint128_t md5;
        uint64_t idx;
        uint64_t owner;
        long len;
    } RegisterFetchFile;

    typedef struct {
        FileMessageType message_type;
        uint128_t md5;
        uint64_t block_idx;
        uint64_t len;
        unsigned char data[];
    } FileContent;
}

#endif //ANONYMOUSP2P_FILEHEADER_HPP
