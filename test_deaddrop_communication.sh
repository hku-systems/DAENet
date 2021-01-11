#!/bin/bash

n=5
for i in `seq $n`
do
        ./build/AnonymousP2P -i 127.0.0.1 -p 410$i -l 127.0.0.1 -o 4101 1> out.$i 2> out.$i &
done

./app/build/PingPongServer 127.0.0.1 41011 socket.config.1 10 1> out.11 2> out.11 &

./app/build/PingPongServer 127.0.0.1 41012 socket.config 10 1 10 1> out.12 2> out.12 &
