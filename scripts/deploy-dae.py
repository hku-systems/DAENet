#!/bin/python
import os
import numpy as np

from remote.remote_execute import unique_execute, execute, connect, run_once, local_execute
import random
import signal
import sys, time
import remote.config as config
from deploy_environment import setup_environment, cleanup_environment
import time
# deployment script for tor/tcp


lib="libdae.so"
branch="master"
n=6
bits = 20
k=0.1
delay=30
request_number=5
msg_rate=500
pool_size=10
server_port=[]
daemon=True
print_shuffle=True

shuffle_P = 0.8

recheck = 20
checkwait = 4

p2p_time_b = 10
p2p_time_a = 1

daemon_suffix=" -e 1" if daemon else ""
apps = ["PingPongServer", "BufferSend"]

cnt_interval = 1 # s
cli_cnt_interval = " --cnt_time %d " % cnt_interval
f_cnt_interval = "cnt_time %d\n" % cnt_interval

cli_p2p_time_b = " --p2p_time_before " + str(p2p_time_b) + " "
cli_p2p_time_a = " --p2p_time_after " + str(p2p_time_a) + " "


app=apps[0]

d_debug = "debug True\n"
cli_debug = " -d 1 "

node_ids = []
with open('node_ids.txt') as f:
    for l in f.readlines():
        node_ids.append(int(l))

cur_cnt = 0

client = []

for i, host in enumerate(config.servers):
    replicas = n
    client.append((host, 3100 + i))
    for idx in range(replicas):
        server_port.append((host, 3100 + idx + i))

debug_shuffle="debug_shuffle true\n" if print_shuffle else ""
cli_debug_shuffle=" --debug_shuffle 1 " if print_shuffle else ""
shuffle = "shuffle"
config_content= "m %d\n" \
                "p2p_time_before %d\n" \
                "p2p_time_after %d\n" \
                "msg_rate %d\n" \
                "pool_size %d\n" \
                "shuffle_P %f\n" \
                "bt_time %d\n" \
                "recheck %d\n" \
                "checkwait %d\n" \
                "%s" \
                "%s" \
                "%s" \
                "shuffle %s\n" \
                "transmission 1\n" % (bits, p2p_time_b, p2p_time_a, msg_rate, pool_size, shuffle_P, delay, recheck, checkwait, debug_shuffle, d_debug, f_cnt_interval, shuffle)

unique_server=set([host for host, port in server_port])
k = int(len(config.servers) * n * k)
print('k=', k)
@run_once
def deploy_dae():
    for host in unique_server:
        with connect(host, "dae-shuffle_with_P", False) as r:
            r.execute("mkdir temp; rm -rf build; rm -rf anonymous-p2p")
    for host in unique_server:
        os.system("scp -r ~/code/anonymous-p2p %s@%s:~/%s-dae-shuffle_with_P/anonymous-p2p" % (config.user, host, host))
    for host in unique_server:
        print(host)
        with connect(host, "dae-shuffle_with_P", False) as r:
            #r.execute("rm -rf ../%s-J_dae-shuffle_with_P" % (host))
            #r.put('../../backup_yunpeng/anonymous-p2p', "./")
            
            #r.execute("git clone git@github.com:jianyu-m/anonymous-p2p.git")
            #r.execute("cd anonymous-p2p; git pull; git checkout %s; git pull" % branch)
            r.execute("cd anonymous-p2p; mkdir -p build; cd build; cmake -DCMAKE_BUILD_TYPE=Debug ../; make -j;make install")
            r.execute("cd anonymous-p2p; mkdir app/build/;cd app/build/;cmake -DCMAKE_BUILD_TYPE=Debug ../; make -j")


def sample_servers():
    # sample client, server pairs
    client_servers = random.sample(server_port, k=k*2)
    #client_servers = client
    static_nodes = list(set(server_port) - set(client_servers))
    for idx in range(k):
        config_file_client = "dae-socket.config.%s-%d" % (client_servers[idx][0], client_servers[idx][1])
        config_file_server = "dae-socket.config.%s-%d" % (client_servers[idx + k][0], client_servers[idx + k][1])
        global cur_cnt
        node_id = node_ids[cur_cnt]
        cur_cnt += 1
        with open(config_file_client, 'w') as f:
            f.write("%s"\
                    "session %d\n"\
                    "seed %d\n" \
                    "key %d\n" \
                    "peer_ip %s\n"\
                    "peer_port %d" % (config_content, idx + 1, idx + 1, node_id, static_nodes[0][0],static_nodes[0][1]))
        node_id = node_ids[cur_cnt]
        # global cur_cnt
        cur_cnt += 1
        with open(config_file_server, 'w') as f:
            f.write("%s"\
                    "server 1\n"\
                    "session %d\n"\
                    "seed %d\n" \
                    "key %d\n" \
                    "peer_ip %s\n"\
                    "peer_port %d" % (config_content, idx + 1, idx + 1, node_id, static_nodes[0][0],static_nodes[0][1]))
        with connect(client_servers[idx][0], "dae-shuffle_with_P", False) as r:
            r.put(config_file_client, "anonymous-p2p/dae-socket.config.%d" % client_servers[idx][1])
        with connect(client_servers[idx + k][0], "dae-shuffle_with_P", False) as r:
            r.put(config_file_server, "anonymous-p2p/dae-socket.config.%d" % client_servers[idx + k][1])
    local_execute("rm dae-socket.config.*")
    return static_nodes, client_servers

executing_process = []

def run_all(static_nodes, client_servers):
    bootstrap_server=static_nodes[0]
    executing_servers=0
    unique_servers = set([node[0] for node in static_nodes])
    server_list = {}
    for server in unique_servers:
        server_list[server] = list()
    
    # print server_list
    for ip, port in static_nodes:
        server_list[ip].append((ip, port))

    for server in server_list:
        # list of servers 
        ll = server_list[server]
        with open("server-execute-%s.sh" % server, 'w') as f:
            for ip, port in ll:
                global cur_cnt
                node_id = node_ids[cur_cnt]
                cur_cnt += 1
                f.write("./build/AnonymousP2P "\
                "-i %s -p %d %s -k %d %s %s --shuffle_P %f --recheck %d --checkwait %d %s %s  -l %s -o %d -m %d --msg_rate %d --pool_size %d --bt_time %d%s\n" 
                % (ip, port, cli_debug, node_id, cli_cnt_interval, cli_debug_shuffle, shuffle_P, recheck, checkwait, cli_p2p_time_b, cli_p2p_time_a,
                bootstrap_server[0], bootstrap_server[1], bits, msg_rate, pool_size, delay, daemon_suffix))
        with connect(server, "dae-shuffle_with_P/anonymous-p2p/", False) as r:
            print("run %d instances" % len(ll))
            r.put("server-execute-%s.sh" % server, "server-execute.sh")
            last_node = r.execute("bash server-execute.sh", will_wait=False)
    
    last_node.wait()
    # for server in static_nodes:
    #     with connect(server[0], "dae-shuffle_with_P/anonymous-p2p/", False) as r:
    #         executing_servers += 1
    #         wait_for = (executing_servers >= 50)
    #         r.execute("./build/AnonymousP2P "\
    #             "-i %s -p %d -l %s -o %d -m %d --msg_rate %d --pool_size %d --bt_time %d%s" 
    #             % (server[0], server[1], 
    #             bootstrap_server[0], bootstrap_server[1], bits, msg_rate, pool_size, delay, daemon_suffix), will_wait=wait_for)
    #         if wait_for:
    #             executing_servers = 0
    for idx in range(k):
        with connect(client_servers[idx][0], "dae-shuffle_with_P/anonymous-p2p/", False) as r:
            r.execute("ulimit -c unlimited; LD_PRELOAD=./lib/%s ./app/build/%s %s %d socket.config.%d %d %d %d" % (lib, app, client_servers[idx][0], client_servers[idx][1], client_servers[idx][1], delay, idx + 1, request_number), False, will_wait=False)
        with connect(client_servers[idx + k][0], "dae-shuffle_with_P/anonymous-p2p/", False) as r:
            r.execute("ulimit -c unlimited; LD_PRELOAD=./lib/%s ./app/build/%s %s %d socket.config.%d %d" % (lib, app, client_servers[idx + k][0], client_servers[idx + k][1], client_servers[idx + k][1], delay), False, will_wait=False)
    time.sleep(delay)
    setup_environment()

def signal_handler(sig, frame):
    unique_execute("killall AnonymousP2P %s" % app, will_wait=False)
    cleanup_environment()
    sys.exit(0)

@run_once
def dae_dep():
    unique_execute("apt-get install build-essential g++ gcc cmake libgoogle-glog-dev libsodium-dev libevent1-dev -y", is_sudo=True)
    

signal.signal(signal.SIGINT, signal_handler)

dae_dep()
deploy_dae()
static_nodes, client_servers = sample_servers()
run_all(static_nodes, client_servers)

print(cur_cnt)
print('k=', k)
time.sleep(100000)