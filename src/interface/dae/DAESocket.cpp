//
// Created by tianxiang on 5/31/19.
//

#include <glog/logging.h>
#include "../interface.hpp"
#include "../c_network.h"
#include "DAESocket.hpp"
#include "relay/RelayConfig.hpp"

class RelayMetadata {
public:
    DAEService* DAERelay;

    uint64_t service_type;

    ring_id_t my_key;

    ring_id_t connector;

    RelayConfig config;

    RelayConfig config_copy;

    //DAE Dialing
    void Dialing(std::string id) {
        //TODO: connector is node id currently. To change it to key, we should make the key-id map identical.
        this->connector = std::stoi(id);

        this->DAERelay->dae_node->connect_service(this->service_type, this->connector, this->my_key);

        // one chance to reconnect
        std::thread([&]{
            std::this_thread::sleep_for(std::chrono::milliseconds(2000));
            if (!this->DAERelay->dae_node->is_service_connected(this->service_type)) {
                this->DAERelay->dae_node->connect_service(this->service_type, connector, this->my_key);
            }
        }).detach();
    }
};

std::string AnonymousRelay::initialize(std::string ip, int port, std::string meta){

    meta = "dae-" + meta;

    metadata = new RelayMetadata();

    metadata->config = RelayConfig::read_from_file(meta);
    metadata->config_copy = RelayConfig::read_from_file(meta);

    metadata->config.relay_ip = ip;
    metadata->config.relay_port = port;
    metadata->config_copy.relay_ip = ip;
    metadata->config_copy.relay_port = port;

    LOG(INFO) << "relay ip: " << ip << " relay port: " << port << " init-peer ip: " << metadata->config.init_peers.ip << " init-peer port: " << metadata->config.init_peers.port;

    uint64_t my_service = message::external_deaddrop_messaging;

    metadata->service_type = my_service;

    metadata->DAERelay = new DAEService(&metadata->config);

    //temporarily relay Identifier is the node_id itself
    metadata->my_key = metadata->DAERelay->node_id;

    // push to service queue
    std::unique_ptr<RelayConfig> cf = std::make_unique<RelayConfig>(std::move(metadata->config_copy));
    metadata->DAERelay->dae_node->ServiceQueue.push(std::move(cf));

    metadata->DAERelay->start();

    return std::to_string(metadata->config.session);
}


/* For ping pong
 * std::unique_ptr<AnonymousSocket> anonymous::AnonymousRelay::connect(std::string id) {
    //Current dialing: client apply for service by directly connecting.
    //An alternative: dialing through dead drop(a third node)

    //this->Dialing(id);
    return std::make_unique<DAESocket>(metadata->DAERelay, metadata->service_type, metadata->my_key, metadata->connector);

}
*/

std::unique_ptr<AnonymousSocket> anonymous::AnonymousRelay::connect(std::string id){
    std::unique_ptr<RelayConfig> Msg = std::move(metadata->DAERelay->dae_node->ServiceQueue.pop_front());
    ConfigFile* config = new ConfigFile(Msg->session, Msg->transmission, Msg->seed);
    return std::make_unique<DAESocket>(metadata->DAERelay, metadata->service_type, metadata->my_key, config);
}

//accept will automatically send a welcome msg to client
std::unique_ptr<AnonymousSocket> anonymous::AnonymousRelay::accept() {
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // TODO: hardcode to avoid client accept
    while (!metadata->config.is_server) {
        sleep(1000);
    }
    DLOG(INFO) << "accept success!!!";
    std::unique_ptr<RelayConfig> Msg = std::move(metadata->DAERelay->dae_node->ServiceQueue.pop_front());
    ConfigFile* config = new ConfigFile(Msg->session, Msg->transmission, Msg->seed);
    std::unique_ptr<AnonymousSocket> soc;
    soc = std::make_unique<DAESocket>(metadata->DAERelay, metadata->service_type, metadata->my_key, config);
    char start[MAX_MTU];
    memset(start, 0, MAX_MTU);
    soc->send(start, 10);
    return soc;

}

int DAESocket::recv(char *buf, int size) {

    DLOG(INFO) << "Node " << this->DAERelayService->node_id << " in recv function, pop!";
    DLOG(INFO) << "Current queue size: " << this->DAERelayService->dae_node->recv_list_user.size();
    std::unique_ptr<AnonymousPacket> packet = std::move(this->DAERelayService->dae_node->recv_list_user.pop_front());

    memcpy(buf, packet->payload(), packet->header()->size);
    DLOG(INFO) << "find msg from local recv queue! content is: " << buf;
    if (!is_active){
        return -1;
    }else if(!strncmp("fin", buf, 3)){
        DLOG(INFO) << "find msg from local recv queue! It's fin !!! ";
        return 8888;
    }else{
        DLOG(INFO) << "find msg from local recv queue! length: " << packet->header()->size;
        return packet->header()->size;
    }

}

int DAESocket::recv(char *buf, int size, int mill)  {
    bool is_timeout = false;
    auto packet = this->DAERelayService->dae_node->recv_list_user.pop_front(mill, is_timeout);
    if (is_timeout) return 0;
    while (packet == nullptr) {
        is_timeout = false;
        packet = this->DAERelayService->dae_node->recv_list_user.pop_front(mill, is_timeout);
        if (is_timeout) return 0;
    }
    if (!is_active) { return -1; }
    memcpy(buf, packet->payload(), packet->header()->size);
    return packet->header()->size;
}

void DAESocket::close() {
    this->is_active = false;
}

//dead drop msging
void DAESocket::send(char* buf, int size){
    ring_id_t deaddrop_id = random.rand() % max_id;
    DLOG(INFO) << "The deaddrop location: " << deaddrop_id;
    DLOG(INFO) << "Secure dead drop communication begin!!!";
    // Msg type & Session ID make the handler different
    //this->DAERelayService->dae_node->send_content_package(false, this->session_ID, this->seq, this->service_type, deaddrop_id, this->my_key, buf, size);
//    LOG(INFO) << "Current seq number is: " << this->seq;
//    this->seq++;

    // Change1: buffer send
    std::unique_ptr<MsgDescriptor> Msg = std::make_unique<MsgDescriptor>(false, this->session_ID, this->seq, this->service_type, deaddrop_id, this->my_key, buf, size);
    this->DAERelayService->dae_node->send_content_package(Msg->info.is_deaddrop,
                                       Msg->info.session_id,
                                       Msg->info.seq_id,
                                       Msg->info.msgtype,
                                       Msg->info.to_id,
                                       Msg->info.from_id,
                                       Msg->info.msg,
                                       Msg->info.size_of_payload);
    DLOG(INFO) << "Push to MsgQueue, current seq number is: " << this->seq;
    this->seq++;
}

ring_id_t anonymous::AnonymousRelay::id_range() {
    return 0xfffff;
}

std::string anonymous::AnonymousRelay::id_to_key(ring_id_t id) {
    return std::to_string(id);
}

ring_id_t anonymous::AnonymousRelay::key_to_id(std::string key) {
    return std::stoi(key);
}