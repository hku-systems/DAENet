
#include "../service.hpp"
#include "PingPongServer.hpp"
#include <interface.hpp>
#include <thread>
#include <glog/logging.h>
#include <cstring>
#include <sodium.h>
#include <string>
#include <chrono>
#include <map>
using namespace anonymous;

bool PingPongServer::init(std::string config_file) {
    key = relay.initialize(service_config.ip, service_config.port, config_file);
    if (key == "")
        return false;
    return true;
}

void PingPongServer::start() {

    while (true) {
        auto socket = relay.accept();
        std::thread([socket = std::move(socket), this](){

            char buf[1000];
            while (true) {
                int len = socket->recv(buf, 1000, 20000);
                if (len < 0) {
                    LOG(INFO) << "request ended";
                    socket->close();
                    break;
                }
                ping_t *ping = (ping_t *) buf;
                if (std::string(ping->id) == key) {
                    socket->send(buf, len);
                } else {
                    socket->send(buf, len);
                    LOG(WARNING) << "ping id error " << "received id " << std::string(ping->id) << " expected: " << key;
                }
                // LOG(INFO) << "handle request ";
            }
        }).detach();

    }
}

void PingPongServer::ping(std::string s, int n) {
    auto socket = relay.connect(s);
    ping_t ping;
    std::strcpy(ping.id, s.c_str());

    unsigned long idx = 1;
    int running = true;
    std::map<int, std::chrono::time_point<std::chrono::steady_clock>> timestamp;
    while (true) {
        unsigned long total_time_ms = 0;
        int i = 0;
        int ii = 0;
        
        while (true) {
            ping.idx = idx;
            timestamp[idx] = std::chrono::steady_clock::now();
            socket->send((char *) &ping, sizeof(ping));
            char buf[1024];
            ping_t *ping_recv = (ping_t*)buf;

            while (true) {
                int len = socket->recv(buf, 1024, 10000);
                if (len == 0) {
                    // timeout
                    LOG(INFO) << "session: " << s << " timeout";
                    idx++;
                    break;
                } else if (len < 0) {
                    running = false;
                    LOG(INFO) << "connection invalid";
                    break;
                }

                // we expected idx should be idx or idx - 1
                if (ping_recv->idx >= idx - 1) {
                    // we receive the expected msg, break the loop to send another message
                    idx++;
                    i++;
                    if (timestamp.count(ping_recv->idx) > 0) {
                        total_time_ms += std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - timestamp[ping_recv->idx]).count();
                        ii++;
                    }
                    break;
                } else {
                    // we received stall message, receive again
                    i++;
                    LOG(INFO) << "session: " << s << " stall packets, idx " << ping_recv->idx << " expected idx " << idx;
                }

            }
            if (i >= n) break;
        }
        // auto duration = std::chrono::duration_cast<std::chrono::milliseconds>
        //         (std::chrono::steady_clock::now() - start);
        if (!running) break;
        LOG(INFO) << "session: " << s << " ping " << n << " times in " << total_time_ms / ii;
    }
    socket->close();
}

int main(int argc, char** argv) {
    if (sodium_init() == -1) {
        abort();
    }
    if (crypto_aead_aes256gcm_is_available() == 0) {
        abort(); /* Not available on this CPU */
    }

    service_t service_config = {
            .ip = std::string(argv[1]),
            .port = std::atoi(argv[2])
    };

    PingPongServer *service = new PingPongServer(service_config);

    std::string config_file = std::string(argv[3]);
    bool succeed = service->init(config_file);
    if (!succeed) {
        return -1;
    }
    sleep(std::atoi(argv[4]));
    auto thread = std::thread([service]{
        service->start();
    });

    if (argc > 5) {
        service->ping(argv[5], std::atoi(argv[6]));
    }
    thread.join();
}