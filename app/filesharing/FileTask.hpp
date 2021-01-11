//
// Created by Maxxie Jiang on 25/3/2019.
//

#ifndef ANONYMOUSP2P_FILETASK_HPP
#define ANONYMOUSP2P_FILETASK_HPP

// Each Download Task is defined as FileTask
// File Task Contain the current maximun sequence_id
// and it is written into file
// if there is a timeout when a file is not arrived
// then a request for the block is requested
#include <queue>
#include "FileHeader.hpp"

typedef struct {
    int idx;
    int size;
    unsigned char* buf;
} FileBlock;

bool operator<(const FileBlock& left, const FileBlock& right);

class FileTask {
private:
    // the md5 value of the file
    message::uint128_t md5;
    // total len in bytes
    long len;

    // the current maximum sequence id
    long cur_seq_id;

    // current_recv_len
    long cur_recv_len;

    // the file name fd
    int fd;

    std::string filename;

    std::priority_queue<FileBlock> un_appended_blocks;
public:
    FileTask() = default;
    explicit FileTask(message::uint128_t _md5, long _len, std::string fname): md5(_md5), len(_len) {
        cur_seq_id = 0;
        cur_recv_len = 0;
        filename = std::move(fname);
    }

    // open the file, and prepare writing to the file
    bool init();

    void append_data_block(int idx, int size, unsigned char* buf);

    bool check_fin();
};


#endif //ANONYMOUSP2P_FILETASK_HPP
