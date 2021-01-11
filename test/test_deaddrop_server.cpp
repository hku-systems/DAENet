//
// Created by tianxiang on 6/1/19.
//
#include <interface/interface.hpp>
#include <thread>
#include <glog/logging.h>
#include <cstring>
#include <sodium.h>
#include <time.h>
#include <sys/time.h>

int main(int argc, char* argv[]) {

    // start up msg
    char start[MAX_MTU] = "Welcome from client!!!";
    // fetcher msg
    char fetcher[MAX_MTU] = "fetch";
    //end up msg
    char fin[MAX_MTU] = "fin";

    anonymous::AnonymousRelay relay;

    struct timeval tv;
    gettimeofday(&tv,NULL);
    LOG(INFO) << "time: start initialize: " << tv.tv_sec*1000 + tv.tv_usec/1000;

    relay.initialize(std::string("127.0.0.1"), std::atoi(argv[2]), "socket.config.1");

    std::this_thread::sleep_for(std::chrono::milliseconds(30000));
    //send
    auto socket = relay.accept();

    struct timeval tv2;
    gettimeofday(&tv2,NULL);
    LOG(INFO) << "time: start sending welcome msg "  << " time: " << tv2.tv_sec*1000 + tv2.tv_usec/1000;


//    for(int i = 0 ; i < 2 ; i++){
//        //Para1: app payload | Para2: size of payload
//
//        socket->send(file, strlen(file));
//    }
//    //recv
//    std::this_thread::sleep_for(std::chrono::milliseconds(10000));

    while(true){
        char buf[1000];
        //Para1: app payload | Para2: size of payload
        int t = socket->recv(buf, strlen(buf));
        if(t == -1){
            struct timeval tv3;
            gettimeofday(&tv3,NULL);
            //LOG(INFO) << "time: t = -1: " << tv3.tv_sec*1000 + tv3.tv_usec/1000;
            //LOG(INFO) << "recv fail";
        }else if(t == 8888){
            struct timeval tv4;
            gettimeofday(&tv4,NULL);
            LOG(INFO) << "time: t = 8888: " << tv4.tv_sec*1000 + tv4.tv_usec/1000;
            LOG(INFO) << "send success! end current session!";
            socket->send(fin, strlen(fin));
            break;
        }else{
            struct timeval tv5;
            gettimeofday(&tv5,NULL);
            LOG(INFO) << "time: t > 0: " << tv5.tv_sec*1000 + tv5.tv_usec/1000;
            LOG(INFO) << "recv success! length: " << t;
            socket->send(fetcher, strlen(fetcher));
        }
    }

    LOG(INFO) << "Process ended.";



//    std::thread([&]{
//    }).detach();

    //You can check receiving by looking into the log

    sleep(10000);

    return 0;
}
