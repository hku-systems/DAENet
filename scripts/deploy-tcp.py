#!/bin/bash

from remote.remote_execute import unique_execute, execute, connect, run_once
import random
import signal
import sys, time
import remote.config as config
from deploy_environment import setup_environment, cleanup_environment

# deployment script for tor/tcp

lib="libtcp.so"
branch="master"
n=10
k=int(len(config.servers) * n * 0.1)
request_number=100
server_port=[]
delay=1
apps = ["PingPongServer", "BufferSend"]
app=apps[0]

for host in config.servers:
    for idx in range(n):
        server_port.append((host, 3100 + idx))

config_file = "tcp-socket.config"
with open(config_file, 'w') as f:
    for server in server_port:
        f.write("%s %d\n" % (server[0], server[1]))

unique_server=set([host for host, port in server_port])

@run_once
def deploy_tcp():
    for host in unique_server:
        with connect(host, "dae-eval", False) as r:
            r.execute("git clone git@github.com:jianyu-m/anonymous-p2p.git")
            r.execute("cd anonymous-p2p; git pull; git checkout %s; git pull" % branch)
            r.execute("cd anonymous-p2p; mkdir -p build; cd build; cmake ../;make -j;make install")
            r.execute("cd anonymous-p2p; mkdir app/build/;cd app/build/;cmake ../; make -j")

@run_once
def update_tcp_config():
    for host in unique_server:
        with connect(host, "dae-eval", False) as r:
            r.put(config_file, "anonymous-p2p/")

executing_process = []

def run_all(client_n):
    clients = set(random.sample(server_port, k=client_n))
    for host, port in server_port:
        with connect(host, "dae-eval/anonymous-p2p/", False) as r:
            if (host, port) in clients:
                while True:
                    dest_id = random.randint(0, len(server_port))
                    if server_port[dest_id][0] != host:
                        break
                dest = "%s %d" % (dest_id, request_number)
                print "%s:%d -> %s:%d" % (host, port, server_port[dest_id][0], server_port[dest_id][1])
            else:
                dest = ""
            process = r.execute("LD_PRELOAD=./lib/%s ./app/build/%s %s %d socket.config %d %s" % (lib, app, host, port, delay, dest), False, will_wait=False)
            executing_process.append(process)

def signal_handler(sig, frame):
    unique_execute("killall %s" % app)
    cleanup_environment()
    sys.exit(0)

signal.signal(signal.SIGINT, signal_handler)

deploy_tcp()
update_tcp_config()
setup_environment()
run_all(k)

time.sleep(1000)
