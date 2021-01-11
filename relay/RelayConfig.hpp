//
// Created by jianyu on 2/20/19.
//

#ifndef ANONYMOUSP2P_RELAYCONFIG_HPP
#define ANONYMOUSP2P_RELAYCONFIG_HPP

#include <string>
#include "Sharp.h"
#include "utils/CLI11.hpp"
#include <fstream>

typedef struct {
    std::string ip;
    int         port;
    ring_id_t   relay_id;
    bool        check_usage = true;
} PeerPair;

typedef std::tuple<int, int> msg_info;

class RelayConfig {
public:
    std::string relay_ip    =   "0.0.0.0";
    int         relay_port  =   3010;
    bool        standalone  =   false;
    bool        debug_shuffle = false;
    std::string shuffle     =   "shuffle";
    PeerPair    init_peers  =   {.ip    =   "10.22.1.16", .port   =   3010, .relay_id = 0};
    int         ring_bits   =   10;        //1M
    int         daemon      =   0;
    int         debug       =   false;
    bool        is_server   =   false;
    int         msg_rate    =   10;
    int         min_pool    =   10;
    int         bt_time     =   10;

    int         cnt_interval = 1; //s
    double      shuffle_P = 0.8; //s
    int recheck = 10;  //
    int checkwait = 2;  //s
    int d_p2p_time_before = 10; //ms
    int d_p2p_time_after = 1; //s

    int send_dummy = false;
    int check_failure = false;

    ring_id_t   relay_id    =   0;
    ring_id_t   max_relay_id = 0;

    int session = 0;
    std::string transmission;
    int64_t seed;

    // format ./Relay ip:port --list=
    static RelayConfig init_from_cmd(int argc, char** argv) {
        auto relayConfig = RelayConfig();
        CLI::App app{"Relay"};

        int debug_int_relay_id;
        app.add_option<std::string>("-i,--ip", relayConfig.relay_ip, "ip address");
        app.add_option<int>("-p,--port", relayConfig.relay_port, "ip port");
        app.add_option<int>("-k,--key", debug_int_relay_id, "node key id");
        app.add_option<int>("-m,--key_range", relayConfig.ring_bits, "number of bits for node keys");
        app.add_option<int>("-e,--daemon", relayConfig.daemon, "runs in daemon");
        app.add_option("-l,--peer_ip", relayConfig.init_peers.ip, "init connected peers ip");
        app.add_option("-o,--peer_port", relayConfig.init_peers.port, "init connected peers port");
        app.add_option("-s,--standalone", relayConfig.standalone, "start with standalone");
        app.add_option("-d,--debug", relayConfig.debug, "debugging mode");
        app.add_option("--msg_rate", relayConfig.msg_rate, "message rate");
        app.add_option("--pool_size", relayConfig.min_pool, "minimum shuffle pool size");
        app.add_option("--bt_time", relayConfig.bt_time, "bootstrap time");
        app.add_option("--cnt_time", relayConfig.cnt_interval, "cnt time");
        app.add_option("--debug_shuffle", relayConfig.debug_shuffle, "debug shuffle");
        app.add_option("--shuffle_P", relayConfig.shuffle_P, "shuffle P");
        app.add_option("--recheck", relayConfig.recheck, "re-check times");
        app.add_option("--checkwait", relayConfig.checkwait, "check wait (s)");
        app.add_option("--p2p_time_before", relayConfig.d_p2p_time_before, "p2p time interval before");
        app.add_option("--p2p_time_after", relayConfig.d_p2p_time_after, "p2p time interval after");
        app.add_option("--check_failure", relayConfig.check_failure, "check failure");
        app.add_option("--send_dummy", relayConfig.send_dummy, "send dummy");
        app.parse(argc, argv);

        relayConfig.relay_id = debug_int_relay_id;
        relayConfig.max_relay_id = (1 << (relayConfig.ring_bits));

        return std::move(relayConfig);
    }

    static RelayConfig read_from_file(std::string filename) {
        RelayConfig config;
        std::ifstream config_file(filename);
        std::string key, value;
        while (config_file >> key >>value) {
            if (key == "peer_ip") {
                config.init_peers.ip = value;
            } else if (key == "peer_port") {
                config.init_peers.port = std::stoi(value);
            } else if (key == "debug") {
                config.debug = true;
            } else if (key == "key") {
                config.relay_id = std::stoi(value);
            } else if (key == "m") {
                config.ring_bits = std::stoi(value);
            } else if (key == "session") {
                config.session = std::stoi(value);
            } else if (key == "transmission") {
                config.transmission = value;
            } else if (key == "seed") {
                config.seed = std::stoi(value);
            } else if (key == "server") {
                config.is_server = true;
            } else if (key == "msg_rate") {
                config.msg_rate = std::stoi(value);
            } else if (key == "pool_size") {
                config.min_pool = std::stoi(value);
            } else if (key == "bt_time") {
                config.bt_time = std::stoi(value);
            } else if (key == "debug_shuffle") {
                config.debug_shuffle = true;
            } else if (key == "shuffle") {
                config.shuffle = value;
            } else if (key == "cnt_time") {
                config.cnt_interval = std::stoi(value);
            } else if (key == "shuffle_P") {
                config.shuffle_P = std::stod(value);
            } else if (key == "recheck") {
                config.recheck = std::stoi(value);
            } else if (key == "checkwait") {
                config.checkwait = std::stoi(value);
            } else if (key == "p2p_time_before") {
                config.d_p2p_time_before = std::stoi(value);
            } else if (key == "p2p_time_after") {
                config.d_p2p_time_after = std::stoi(value);
            } else if (key == "send_dummy") {
                config.send_dummy = std::stoi(value);
            } else if (key == "check_failure") {
                config.check_failure = std::stoi(value);
            }
//            else if (key == "relay_ip") {
//                config.relay_ip = std::stoi(value);
//            }else if (key == "relay_port") {
//                config.relay_port = std::stoi(value);
//            }
        }
        config.max_relay_id = (1 << (config.ring_bits));
        return config;
    }

    RelayConfig(){}
};


#endif //ANONYMOUSP2P_RELAYCONFIG_HPP
