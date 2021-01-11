//
// Created by tianxiang on 5/31/19.
//

#include <interface/interface.hpp>
#include <thread>
#include <glog/logging.h>
#include <cstring>
#include <sodium.h>
int main(int argc, char* argv[]) {

    anonymous::AnonymousRelay relay;

    relay.initialize(std::string("127.0.0.1"), 4101, "socket.config");

    auto socket = relay.accept();

    char buf[1024];
    socket->recv(buf, 1024);
    LOG(INFO) << "Well recved!!!!!!!!!!!! MSG recved from client is: " << buf;

    sleep(10000);

    return 0;
}
