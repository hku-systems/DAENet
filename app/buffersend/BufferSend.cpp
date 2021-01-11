//
// Created by jianyu on 6/7/19.
//

#include "BufferSend.hpp"
#include <cstring>

typedef struct {
    char id[128];
} fetch_t;

void BufferSend::fetch(std::string s, int n) {
    auto socket = relay.connect(s);
    fetch_t fetch;
    std::strcpy(fetch.id, s.c_str());

    // concurrent sending n packets
    for (int i = 0;i < n;i++) {
        socket->send((char *) &fetch, sizeof(fetch));
    }

    int recv_count = 0;

    auto start = std::chrono::steady_clock::now();

    while (true) {
        char buf[MAX_MTU];
        int len = socket->recv(buf, 1024, 10000);
        if (len == 0) {
            // timeout
            LOG(INFO) << "session: " << s << " timeout";
            socket->send((char *) &fetch, sizeof(fetch));
            continue;
        } else if (len < 0) {
            LOG(INFO) << "connection invalid";
            break;
        }
        recv_count += 1;
        socket->send((char *) &fetch, sizeof(fetch));
        if (recv_count == n) {
            auto now = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>
                    (now - start);
            start = now;
            recv_count = 0;
            LOG(INFO) << "session: " << s << " ping " << n << " times in " << duration.count();
        }
    }

    socket->close();
}


void BufferSend::start() {

    while (true) {
        auto socket = relay.accept();
        std::thread([socket = std::move(socket), this](){

            char buf[MAX_MTU];
            while (true) {
                int len = socket->recv(buf, 1000);
                if (len <= 0) {
                    LOG(INFO) << "request ended";
                    socket->close();
                    break;
                }
                fetch_t *ping = (fetch_t *) buf;
                if (std::string(ping->id) == key) {
                    socket->send(buf, 500);
                } else {
                    LOG(WARNING) << "ping id error";
                }
                // LOG(INFO) << "handle request ";
            }
        }).detach();

    }
}

bool BufferSend::init(std::string config) {
    key = relay.initialize(service_config.ip, service_config.port, config);
    if (key == "")
        return false;
    return true;
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

    auto *service = new BufferSend(service_config);

    bool succeed = service->init(std::string(argv[3]));
    if (!succeed) {
        return -1;
    }

    sleep(std::atoi(argv[4]));
    auto thread =  std::thread([service]{
        service->start();
    });

    if (argc > 5) {
        service->fetch(argv[5], std::atoi(argv[6]));
    }
    thread.join();
}