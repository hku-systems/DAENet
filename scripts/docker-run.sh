#!/bin/bash

# MASTER_PORT
# MASTER_IP
# IPADDR
# PORT

# TODO: here the config file is not read
source config.sh

if [ -z "${ROLE}" ]; then
    ROLE="MASTER"
fi

if [ -z "${MASTER_IP}" ]; then
    MASTER_IP=127.0.0.1
fi

if [ -z "${MASTER_PORT}" ]; then
    MASTER_PORT=3010
fi

if [ -z "${PORT}" ]; then
    PORT=3010
fi

if [ -z "${IPADDR}" ]; then
    IPADDR=0.0.0.0
fi

IP=$(ip a | grep -oE "\b([0-9]{1,3}\.){3}[0-9]{1,3}\b" | grep 10.1|head -n 1)

IPADDR=$IP

if [ "${STANDALONE}" == "STANDALONE" ]; then
    ALONE="--standalone=1"
else
    ALONE=
fi

if [ "$ROLE" == "MASTER" ]; then
    echo "Starting Master Node $IPADDR:$PORT"
    ../build/AnonymousP2P --ip=$IPADDR --port=$PORT $ALONE
else
    echo "Starting Slave Node $IPADDR:$PORT -> $MASTER_IP:$MASTER_PORT"
    ../build/AnonymousP2P --ip=$IPADDR --port=$PORT --peer_ip=$MASTER_IP --peer_port=$MASTER_PORT $ALONE
fi