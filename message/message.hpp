//
// Created by jianyu on 2/10/19.
//

#ifndef ANONYMOUSP2P_MESSAGE_HPP
#define ANONYMOUSP2P_MESSAGE_HPP

#endif //ANONYMOUSP2P_MESSAGE_HPP

namespace message {
    enum {
        dummy_message = 0xfff1,
        //fetch_relay = 0xfff0,
        internal_join_find_successor = 0xfff2,
        internal_reply_find_successor = 0xfff3,
        internal_find_predecessor = 0xfff4,
        internal_reply_get_predecessor = 0xfff5,
        internal_notify_predecessor_join = 0xfff6,
        internal_fetch_successor_list = 0xfff7,
        internal_reply_successor_list = 0xfff8,
        internal_check_predecessor_existence = 0xfff9,
        internal_reply_predecessor_existence = 0xfffa,
        internal_check_successor_existence = 0xfffb,
        internal_reply_successor_existence = 0xfffc,
        external_pingpong_service = 0xfffd,
        external_reply_pingpong_service = 0xfffe,
        external_deaddrop_messaging = 0xffff,
        external_reply_deaddrop_messaging = 0x10000
    };
}

// neighbour discovery process:
// 1. start a bootstrap server
// 2. another server connect to the bootstrap server
//  2.1 add it to the current server
//  2.2 randomly send to the next server
//  2.3 if it is the sender, then ends
//  2.4 if not, jump to 2.1