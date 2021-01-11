#!/bin/bash

n=2
for i in `seq $n`
do
        ./AnonymousP2P 10.22.1.16 410$i 1> out.$i 2> out.$i &
done
