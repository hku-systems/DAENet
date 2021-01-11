//
// Created by jianyu on 3/18/19.
//

#include <cstring>
#include <fcntl.h>
#include <thread>
#include "FileService.hpp"
#include "md5.h"
#include "interface.hpp"
#include <glog/logging.h>

void message::set_md5_from_buf(const message::uint128_t &md5, const char *buf)  {
    std::memcpy((void*)(&md5), buf, 16);
}

bool message::operator<(const message::uint128_t &left, const message::uint128_t &right) {
    return (left.h != right.h)? left.h < right.h : left.l < right.l;
}

std::istream& message::operator>>(std::istream& istream, uint128_t& md5) {
    return istream >> std::hex >> md5.h >> std::hex >> md5.l;
}

std::ostream& message::operator<<(std::ostream& os, const uint128_t& md5) {
    return os << std::hex << md5.h << " " << std::hex << md5.l;
}

void FileService::start() {
    AnonymousRelay relay;
    auto cur_key = relay.initialize(service_config.ip, service_config.port, "socket.config");
    
    auto thread = std::thread([&]() {
        while (true) {
            // assume a fetch
            auto socket = relay.accept();
            if (!socket->is_valid()) { break; }
            std::thread([socket = std::move(socket), this]() {
                char buf[1500];
                while (true) {
                    int size = socket->recv(buf, 1500);
                    if (size <= 0)
                        break;
                    char *recv_msg = (char*) buf;
                    message::FileMessageType messageType;
                    auto fetch_register = reinterpret_cast<message::RegisterFetchFile*>(recv_msg);
                    messageType = fetch_register->message_type;
                    switch (messageType) {
                        case message::register_file: {
                            auto md5 = fetch_register->md5;
                            auto owner = fetch_register->owner;
                            auto len = fetch_register->len;
                            registered_file_t registered_file = {.file_len = len, .file_owner = owner};
                            registered_files[md5] = registered_file;
                            fetch_register->message_type = message::register_file_reply;
                            // send reply
                            socket->send(buf, sizeof(message::RegisterFetchFile));
                            LOG(INFO) << "register file " << fetch_register->md5 << " in this hop";
                            break;
                        }
                        case message::register_file_reply:
                            // ignore the reply
                            LOG(INFO) << "file " << fetch_register->md5 << " registered";
                            break;
                        case message::fetch_file: {
                            auto md5 = fetch_register->md5;
                            auto idx = fetch_register->idx;

                            //TODO: only when requested_len is 0, the whole file is requested
                            auto requested_len = fetch_register->len;

                            open_file_t file = shared_files[md5];

                            uint64_t current_idx = idx;
                            lseek(file.fd, current_idx, SEEK_SET);

                            char file_buf[1500];
                            while (true) {
                                auto content = reinterpret_cast<message::FileContent *>(file_buf);
                                int len = read(file.fd, content->data, 1300);
                                if (len <= 0)
                                    break;
//                                LOG(INFO) << "fetch file " << fetch_register->md5 << " idx " << current_idx;
                                content->len = len;
                                content->message_type = message::file_content;
                                content->block_idx = current_idx;
                                content->md5 = md5;
                                socket->send(file_buf, sizeof(message::FileContent) + len);
                                current_idx += len;
                            }

                            socket->close();
                            break;
                        }
                        case message::fetch_file_loc: {
                            auto md5 = fetch_register->md5;
                            auto file = registered_files[md5];

                            fetch_register->owner = file.file_owner;
                            fetch_register->len = file.file_len;
                            fetch_register->message_type = message::fetch_file_loc_reply;
                            socket->send(buf, sizeof(message::RegisterFetchFile));
                            LOG(INFO) << "request file loc " << fetch_register->md5 << "->" << file.file_owner << " len:" << file.file_len;
                            break;
                        }
                        default:
                            LOG(ERROR) << "unknown message";
                    }
                }
            }).detach();
        }
    });

    thread.detach();

    char buf[1500];
    while (true) {
        std::string filename, msg;
        std::cin >> msg;
        message::uint128_t md5_buf = {.h = 0, .l = 0};
        char file_buf[1500];
        uint64_t total_len;
        if (msg == "upload") {
            std::cin >> filename;
            total_len = 0;
            // open the file and put it in the open file list
            int fd = open(filename.c_str(), O_RDONLY);
            if (fd < 0) {
                LOG(ERROR) << "open file fail";
                continue;
            }
            open_file_t file = {.fd = fd, .file_len = 0};

            Chocobo1::MD5 md5;
            while (true) {
                int len = read(fd, file_buf, 1500);
                if (len <= 0)
                    break;
                total_len += len;
                // TODO: will this clear the buf?
                md5.addData(file_buf, len);
            }
            md5.finalize();
            message::set_md5_from_buf(md5_buf, md5.raw_value());
            LOG(INFO) << "md5 is " << md5_buf;
            shared_files[md5_buf] = file;

            auto register_file = reinterpret_cast<message::RegisterFetchFile*>(buf);
            register_file->message_type = message::register_file;
            register_file->owner = std::stoi(cur_key);
            register_file->md5 = md5_buf;
            register_file->len = total_len;

            auto connect_socket = relay.connect(message::hash(md5_buf));
            connect_socket->send(buf, sizeof(message::RegisterFetchFile));
            connect_socket->recv(buf, 1500);
            connect_socket->close();
            //send(std::move(packet), md5_buf.l, sizeof(message::RegisterFetchFile));
        } else if (msg == "download") {
            // md5
            std::cin >> md5_buf;

            clock_t begin = clock();

            auto connect_socket = relay.connect(message::hash(md5_buf));

            auto fetch_file = reinterpret_cast<message::RegisterFetchFile*>(buf);

            fetch_file->message_type = message::fetch_file_loc;
            fetch_file->md5 = md5_buf;
            connect_socket->send(buf, sizeof(message::RegisterFetchFile));
            connect_socket->recv(buf, 1500);

            uint64_t dest = fetch_file->owner;

            if (fetch_file->len == 0) {
                LOG(INFO) << "cannot find the file " << md5_buf;
                continue;
            }

            fetch_file->message_type = message::fetch_file;
            fetch_file->md5 = md5_buf;
            fetch_file->idx = 0;

            connect_socket->close();

            FileTask task = FileTask(fetch_file->md5, fetch_file->len, "name");

            auto content_socket = relay.connect(std::to_string(dest));

            content_socket->send(buf, sizeof(message::RegisterFetchFile));

            int size = 1;
            while (size > 0) {
                size = content_socket->recv(buf, 1500);
            }

            clock_t end = clock();

            LOG(INFO) << "download file in " << double(end - begin) / CLOCKS_PER_SEC;

        } else if (msg == "exit") {
            return;
        } else if (msg == "wait") {
            thread.join();
        } else {
            std::cout << "error msg " << msg << std::endl;
        }
    }
}

int main(int argc, char** argv) {
    int port = std::atoi(argv[1]);
    service_t service_config = {
            .ip = "127.0.0.1",
            .port = port
    };
    AnonymousService *service = new FileService(service_config);
    service->start();
}