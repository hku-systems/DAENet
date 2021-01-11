#!/bin/bash

from remote.remote_execute import unique_execute, execute, connect, run_once
import random
import signal
import sys, time
import remote.config as config
from remote.config import servers
from deploy_environment import setup_environment, cleanup_environment
from loopix_backend import deploy_backend, run_backend, update_config
# deployment script for tor/tcp

lib="libloopix.so"
branch="master"
n=10
k=5
request_number=10
server_port=[]

apps = ["PingPongServer", "BufferSend"]
app=apps[0]
delay=10

for host in config.servers:
    for idx in range(n):
        server_port.append((host, 3100 + idx * 3 ))

mix_port = 9999
provider_port = 9990

backends = [
    [
        {'host': servers[0], 'port': mix_port, 'name': "mix-1"},
        {'host': servers[1], 'port': mix_port, 'name': "mix-2"},
        {'host': servers[2], 'port': mix_port, 'name': "mix-3"},
        {'host': servers[3], 'port': mix_port, 'name': "mix-4"},
        {'host': servers[4], 'port': mix_port, 'name': "mix-5"},
    ],

    [
        {'host': servers[5], 'port': provider_port, 'name': "pro-1"},
        {'host': servers[6], 'port': provider_port, 'name': "pro-2"},
        {'host': servers[7], 'port': provider_port, 'name': "pro-3"},
    ],

    [ {'host': host, 'port': port, 'name': "client-%s" % port} for idx, (host, port) in enumerate(server_port)]
]


config_file = "loopix-socket.config"
with open(config_file, 'w') as f:
    for server in server_port:
        f.write("%s %d\n" % (server[0], server[1]))

unique_server=set([host for host, port in server_port])

@run_once
def deploy_loopix_frontend():
    for host in unique_server:
        with connect(host, "dae-eval", False) as r:
            r.execute("git clone git@github.com:jianyu-m/anonymous-p2p.git")
            r.execute("cd anonymous-p2p; git checkout %s; git pull" % branch)
            r.execute("cd anonymous-p2p; mkdir -p build; cd build; cmake ../;make -j;make install")
            r.execute("cd anonymous-p2p; mkdir app/build/;cd app/build/;cmake ../; make -j")
            r.put(config_file, "anonymous-p2p/")

@run_once
def deploy_loopix_backend():
    deploy_backend(backends)

@run_once
def update_loopix_config():
    update_config(backends)

executing_process = []

def run_all(client_n):
    clients = set(random.sample(server_port, k=client_n))
    for host, port in server_port:
        with connect(host, "dae-eval/anonymous-p2p/", False) as r:
            if (host, port) in clients:
                dest_idx =  random.randint(0, len(server_port))
                dest = "%s %d" % (dest_idx, request_number)
                print "%s:%d -> %d (%s:%d)" % (host, port, dest_idx, server_port[dest_idx][0], server_port[dest_idx][1])
            else:
                dest = ""
            process = r.execute("LD_PRELOAD=./lib/%s ./app/build/%s %s %d socket.config %d %s" % (lib, app, host, port, delay, dest), False, will_wait=False)
            executing_process.append(process)

@run_once
def loopix_dep():
    unique_execute("apt-get install python-pip build-essential g++ gcc python-cffi -y", is_sudo=True)
    unique_execute("pip install --user pytest")
    unique_execute("pip install --user numpy twisted petlib fabric sphinxmix==0.0.6")

def loopix_deploy():
    deploy_loopix_backend()
    update_loopix_config()
    deploy_loopix_frontend()

def signal_handler(sig, frame):
    unique_execute("killall %s twistd" % app)
    cleanup_environment()
    sys.exit(0)

signal.signal(signal.SIGINT, signal_handler)

loopix_dep()
loopix_deploy()
setup_environment()
run_backend(backends)
run_all(k)

time.sleep(100000)