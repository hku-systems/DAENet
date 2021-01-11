//
// Created by Maxxie Jiang on 28/1/2019.
//

#ifndef ANONYMOUSP2P_ANONYMOUSPACKET_HPP
#define ANONYMOUSP2P_ANONYMOUSPACKET_HPP

#include <memory>
#include "../Sharp.h"

// | id (uint32) |

typedef struct {
    uint64_t    id;             // service id, random generated
    int         size;           // size of the payload
    uint64_t    relay_id;       // sender relay id, 0 means hidden
    char        relay_ip[20];       // sender relay ip
    uint64_t    relay_dest_id;  // receiver relay id, 0 means random relay
    char        relay_dest_ip[20];  // receiver relay ip
    int         relay_dest_port;    // receiver relay port
    uint64_t    relay_reply_id; // who should reply to
    char        relay_reply_ip[20]; // who should reply to
    int         relay_reply_port;    // reply relay port
    uint64_t    key_id;            // find/update the successor of key
    char        key_ip[20];         // ip of key(node)
    int         key_port;    // port of key(node)
    int         index;               // update reply_relay_id.finger_table[index]
    bool        msg_type;              // internal msg(true), external/application msg(false)
    bool        is_dest;         //for external key-id mapping
    int         session_id;      //for marking one service(Pair<Client,Server>)

} anonymous_header_t;

class AnonymousPacket {
public:
    char buf[MAX_MTU];

    AnonymousPacket(): AnonymousPacket(0, 0, 0) {}

    AnonymousPacket(uint64_t _id, uint64_t src, uint64_t dest) {
        header()->id = _id;
        header()->relay_id = src;
        header()->relay_dest_id = dest;
        header()->size = 0;
    }

    AnonymousPacket(AnonymousPacket& packet) {
        header()->id = packet.header()->id;
        header()->relay_id = packet.header()->relay_id;
        header()->relay_dest_id = packet.header()->relay_dest_id;
        header()->size = packet.header()->size = 0;
        header()->msg_type = packet.header()->msg_type;
    }

    AnonymousPacket(uint64_t _id, uint64_t src, uint64_t dest, bool _is_internal_msg) {
        header()->id = _id;
        header()->relay_id = src;
        header()->relay_dest_id = dest;
        header()->size = 0;
        header()->msg_type = _is_internal_msg;
    }

    AnonymousPacket(uint64_t _id,
            const char* s_ip, uint64_t s_id,
            const char* r_ip, uint64_t r_id, int r_port,
            const char* reply_ip, uint64_t reply_id, int reply_port,
            const char* k_ip, uint64_t k_id, int k_port,
            int _index, bool _is_internal_msg) {

        header()->id = _id;
        header()->relay_id = s_id;
        strncpy(header()->relay_ip, s_ip, strlen(s_ip) + 1);
        header()->relay_dest_id = r_id;
        strncpy(header()->relay_dest_ip, r_ip, strlen(r_ip) + 1);
        header()->relay_dest_port = r_port;
        header()->relay_reply_id = reply_id;
        strncpy(header()->relay_reply_ip, reply_ip, strlen(reply_ip) + 1);
        header()->relay_reply_port = reply_port;
        header()->key_id = k_id;
        strncpy(header()->key_ip, k_ip, strlen(k_ip) + 1);
        header()->key_port = k_port;
        header()->index = _index;
        header()->size = 0;
        header()->msg_type = _is_internal_msg;
    }

    AnonymousPacket(uint64_t _id,
                    const char* s_ip, uint64_t s_id,
                    const char* r_ip, uint64_t r_id, int r_port,
                    const char* reply_ip, uint64_t reply_id, int reply_port,
                    const char* k_ip, uint64_t k_id, int k_port,
                    int _index, bool _is_internal_msg,
                    const char* package, int size,
                    bool _is_dest, int session) {

        header()->id = _id;
        header()->relay_id = s_id;
        strncpy(header()->relay_ip, s_ip, strlen(s_ip) + 1);
        header()->relay_dest_id = r_id;
        strncpy(header()->relay_dest_ip, r_ip, strlen(r_ip) + 1);
        header()->relay_dest_port = r_port;
        header()->relay_reply_id = reply_id;
        strncpy(header()->relay_reply_ip, reply_ip, strlen(reply_ip) + 1);
        header()->relay_reply_port = reply_port;
        header()->key_id = k_id;
        strncpy(header()->key_ip, k_ip, strlen(k_ip) + 1);
        header()->key_port = k_port;
        header()->index = _index;
        header()->size = size;
        header()->msg_type = _is_internal_msg;
        header()->is_dest = _is_dest;
        header()->session_id = session;

        memcpy(buf + sizeof(anonymous_header_t), package, size);
    }

    inline anonymous_header_t* header() {
        return reinterpret_cast<anonymous_header_t*>(buf);
    }

    const char* payload() {
        return buf + sizeof(anonymous_header_t);
    }

    inline int payload_size() {
        return header()->size;
    }

    inline int total_size() { return sizeof(anonymous_header_t) + payload_size(); }

    inline bool is_internal_msg(){
        return header()->msg_type;
    }

    inline void set_service_ID(uint64_t s_id){
        header()->id = s_id;
    }


};


#endif //ANONYMOUSP2P_ANONYMOUSPACKET_HPP
