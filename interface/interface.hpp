//
// Created by jianyu on 6/1/19.
//

#ifndef ANONYMOUSP2P_INTERFAC_HPP
#define ANONYMOUSP2P_INTERFAC_HPP
//
// Created by jianyu on 5/21/19.
//

#ifndef ANONYMOUSP2P_INTERFACE_HPP
#define ANONYMOUSP2P_INTERFACE_HPP


#include <string>
#include "Sharp.h"

class RelayMetadata;

namespace anonymous {

    class AnonymousSocket {
    public:
        int sockfd;

        explicit AnonymousSocket(int fd): sockfd(fd) {}

        // send a buf with the size
        virtual void send(char* buf, int size) = 0;

        // recv the buf with a maximum len of size
        virtual int recv(char* buf, int size) = 0;

        // wait for timeout to receive a message
        virtual int recv(char* buf, int size, int timeout) {
            return recv(buf, size);
        }

        virtual void close() = 0;

        inline bool is_valid() { return sockfd != -1; }

    };

    class AnonymousRelay {
    public:

        static AnonymousRelay *current;
        int sockfd;

        RelayMetadata *metadata;

        // init the anonymous socket and return the key of the anonymous socket
        std::string initialize(std::string ip, int port, std::string meta);

        // connect to a remote with the id
        std::unique_ptr<AnonymousSocket> connect(std::string id);

        // accept a new connection
        std::unique_ptr<AnonymousSocket> accept();

        static ring_id_t key_to_id(std::string key);

        static std::string id_to_key(ring_id_t id);

        static ring_id_t id_range();

    };
}

#endif //ANONYMOUSP2P_INTERFACE_HPP

#endif //ANONYMOUSP2P_INTERFAC_HPP
