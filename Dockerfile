
FROM ubuntu:16.04

RUN apt-get update

RUN apt-get install -y cmake libsodium-dev build-essential g++ libgoogle-glog-dev git

COPY ./ /anonymous-network

RUN rm -rf /anonymous-network/build

RUN mkdir /anonymous-network/build/

WORKDIR /anonymous-network/build/

RUN cmake ..
RUN make

EXPOSE 3010

ENTRYPOINT [ "/anonymous-network/scripts/docker-run.sh", "--ip=0.0.0.0", "--port=3010", "--standalone=1"]
