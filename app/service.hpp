
#ifndef ANONYMOUSP2P_SERVICE_HPP
#define ANONYMOUSP2P_SERVICE_HPP

#include <string>

namespace anonymous {

    typedef struct {
        std::string ip;
        int port;
    } service_t;

    class AnonymousService {
    public:
        service_t service_config;
        AnonymousService(const service_t &service) {
            service_config = service;
        }
        virtual void start() = 0;
    };

}
#endif //ANONYMOUSP2P_SERVICE_HPP
