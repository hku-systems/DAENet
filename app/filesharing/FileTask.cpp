//
// Created by Maxxie Jiang on 25/3/2019.
//

#include "FileTask.hpp"
#include <memory>
#include <cstring>
#include <glog/logging.h>
bool operator<(const FileBlock& left, const FileBlock& right) {
    return left.idx > right.idx;
}

bool FileTask::init() {
    // init fd
    fd = 0;
    return true;
}

void FileTask::append_data_block(int idx, int size, unsigned char *buf) {
    LOG(INFO) << "recv block " << idx << " " << size << " for file " << md5;
    cur_recv_len += size;
    if (idx == cur_seq_id) {
        cur_seq_id += size;

        // check for un appended file
        while (!un_appended_blocks.empty()) {
            const FileBlock &block = un_appended_blocks.top();
            if (block.idx == cur_seq_id) {
                cur_seq_id += block.size;
                delete[] block.buf;
                un_appended_blocks.pop();
            } else if (block.idx < cur_seq_id) {
                // error
                LOG(ERROR) << "invalid block";
            } else {
                break;
            }
        }

    } else {
        auto buff = new unsigned char[size];
        std::memcpy(buff, buf, size);
        FileBlock block = {.idx = idx, .size = size, .buf = buff};
        un_appended_blocks.push(block);
    }
}

bool FileTask::check_fin() {
    std::cout << "downloading file " << md5 << ", " << (double)cur_recv_len / len * 100 << "%" << std::endl;
    if (cur_seq_id == len) {
        // fin
        // close fd
        LOG(INFO) << "file " << md5 << " fin";
        return true;
    }
    return false;
}
