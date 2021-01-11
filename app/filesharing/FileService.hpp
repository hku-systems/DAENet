//
// Created by jianyu on 3/18/19.
//

#ifndef ANONYMOUSP2P_FILESERVICE_HPP
#define ANONYMOUSP2P_FILESERVICE_HPP


// FileHash -> PublicKey -> nodeID
// for now we ignore the publicKey Mapping

// workflow of the file_sharing app
// file_owner -> get the md5 of the file -> get the nodeID -> register it to the node (register_file)
// file_fetcher -> get the md5 of the file -> get the nodeID (fetch_file_loc)
//              -> fetch the file_content to the nodeID (fetch_file)
//              <- (file_content)

#include "AnonymousP2p.hpp"
#include "FileHeader.hpp"
#include "FileTask.hpp"
#include "../service.hpp"
#include <map>

using namespace anonymous;

class FileService: public AnonymousService {
private:
    typedef struct {
        int fd;
        uint64_t file_len;
    } open_file_t;

    typedef struct {
        uint64_t file_len;
        uint64_t file_owner;
    } registered_file_t;

    std::map<message::uint128_t, registered_file_t> registered_files;
    std::map<message::uint128_t, open_file_t> shared_files;

public:
    explicit FileService(const service_t &service): AnonymousService(service) {}
    void start() override;
};


#endif //ANONYMOUSP2P_FILESERVICE_HPP
