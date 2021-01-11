#!/bin/bash

export SLAVES_NUM=10

source config.sh

if [ "$1" == "stop" ]; then
    docker stack rm ano
elif [ "$1" == "compose" ]; then
    docker-compose up
else
    docker stack deploy -c docker-compose.yml ano
fi
