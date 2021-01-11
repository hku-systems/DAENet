//
// Created by tianxiang on 6/1/19.
//

#include <interface/dae/DAESocket.hpp>
#include <thread>
#include <glog/logging.h>
#include <cstring>
#include <sodium.h>
#include <time.h>
#include <sys/time.h>

int main(int argc, char* argv[]) {

    // normal msg
    char file[MAX_MTU] = "We are the Systems Research Group";
    // end up msg
    char fin[MAX_MTU] = "fin";

    anonymous::AnonymousRelay relay;

    struct timeval tv;
    gettimeofday(&tv,NULL);
    LOG(INFO) << "time: start initialize: " << tv.tv_sec*1000 + tv.tv_usec/1000;

    relay.initialize(std::string("127.0.0.1"), std::atoi(argv[2]), "socket.config");

    auto socket = relay.connect(std::string("1"));

    //send
    std::this_thread::sleep_for(std::chrono::milliseconds(30000));

    for(int i = 0 ; i < 3 ; i++){
        //Para1: app payload | Para2: size of payload
        struct timeval tv2;
        gettimeofday(&tv2,NULL);
        LOG(INFO) << "time: start sending msg " << i << " time: " << tv2.tv_sec*1000 + tv2.tv_usec/1000;
        socket->send(file, strlen(file));
    }
    struct timeval tv3;
    gettimeofday(&tv3,NULL);
    LOG(INFO) << "time: start sending fin msg "  << " time: " << tv3.tv_sec*1000 + tv3.tv_usec/1000;
    //server close channel to client
    socket->send(fin, strlen(fin));
    //client close channel to server
    socket->send(fin, strlen(fin));

    //recv
    //std::this_thread::sleep_for(std::chrono::milliseconds(20000));

//    for(int i = 0 ; i < 2 ; i++){
//        //Para1: app payload | Para2: size of payload
//        char buf[1000];
//        int t = socket->recv(buf, strlen(buf));
//        if(t > 0){
//            LOG(INFO) << "recv success! length: " << t;
//        }else{2
//            LOG(INFO) << "recv fail";
//        }
//    }

    while(true){
        char buf[1000];
        //Para1: app payload | Para2: size of payload
        int t = socket->recv(buf, strlen(buf));
        if(t == -1){
            struct timeval tv4;
            gettimeofday(&tv4,NULL);
            //LOG(INFO) << "time: t = -1: " << tv4.tv_sec*1000 + tv4.tv_usec/1000;
            //LOG(INFO) << "recv fail";
        }else if(t == 8888){
            struct timeval tv5;
            gettimeofday(&tv5,NULL);
            LOG(INFO) << "time: t = 8888: " << tv5.tv_sec*1000 + tv5.tv_usec/1000;
            LOG(INFO) << "send success! end current session!";
            break;
        }else{
            struct timeval tv6;
            gettimeofday(&tv6,NULL);
            LOG(INFO) << "time: t > 0: " << tv6.tv_sec*1000 + tv6.tv_usec/1000;
            LOG(INFO) << "recv success! length: " << t;
        }
    }

    LOG(INFO) << "Process ended.";
    //You can check receiving by looking into the log

    sleep(10000);

    return 0;
}
