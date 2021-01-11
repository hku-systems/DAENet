#include <iostream>
#include <memory>
#include <thread>
#include "utils/SharpTunnel.h"
#include "relay/RelayConfig.hpp"
#include <p2p/DAEProtocol.hpp>
#include <p2p/DAEService.hpp>
#include <map>
#include <netinet/in.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include "utils/Daemon.hpp"

int main(int argc, char* argv[]) {

    RelayConfig config = config.init_from_cmd(argc, argv);

    std::string filename = config.relay_ip + "-" + std::to_string(config.relay_port) + ".log"; 
    google::InitGoogleLogging(argv[0]);
    google::SetLogDestination(google::GLOG_INFO, (filename + ".INFO").c_str());
    google::SetLogDestination(google::GLOG_ERROR, (filename + ".ERROR").c_str());
    google::SetLogDestination(google::GLOG_FATAL, (filename + ".FATAL").c_str());

    //Node join
    LOG(INFO) << "service " << config.relay_ip;

    if (config.daemon)
        run_as_daemon(config.relay_ip, config.relay_port);

    auto DAE_service = new DAEService(&config);
    DAE_service->start();

    google::FlushLogFiles(google::GLOG_INFO);

    sleep(100000);

    return 0;
}


