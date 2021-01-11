#!/bin/bash

source config.sh

docker run -e ROLE=SLAVE -e MASTER_IP=10.1.0.3 --network ano_anop2p $REGISTRY/$IMAGE:$VERSION