FROM ubuntu:latest

WORKDIR /opt

RUN apt-get update && \
    apt-get install -y git build-essential autoconf libtool make

# Install jsonnet
RUN git clone https://github.com/google/jsonnet && \
    cd jsonnet && \
    make && \
    cd ..
