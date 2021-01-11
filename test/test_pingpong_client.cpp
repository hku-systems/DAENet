//
// Created by tianxiang on 5/31/19.
//

#include <interface/interface.hpp>
#include <thread>
#include <glog/logging.h>
#include <cstring>
#include <sodium.h>

int main(int argc, char* argv[]) {

    LOG(INFO) << "main log 1";
    anonymous::AnonymousRelay relay;
    LOG(INFO) << "main log 2";
//    RelayConfig config = config.init_from_cmd(argc, argv);

    LOG(INFO) << "main log 3";
    relay.initialize(std::string("127.0.0.1"), 4101, "socket.config");

    LOG(INFO) << "main log 4";
    auto socket = relay.connect("0");

    LOG(INFO) << "main log 5";
    std::thread([&]{
        std::this_thread::sleep_for(std::chrono::milliseconds(5000));
        LOG(INFO) << "main log 6";
        socket->send("Welcome", 1024);
    }).detach();

    LOG(INFO) << "main log 7";
    std::thread([&]{
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        LOG(INFO) << "main log 8";
        char buf[1024];
        socket->recv(buf, 1024);
        LOG(INFO) << "Well recved!!!!!!!!!!!! MSG recved from server is: " << buf;
    }).detach();

    sleep(10000);

    return 0;
}
