# Dockerfile
FROM ubuntu:latest

# Install g++, make and ncat
RUN apt-get update && apt-get install -y \
    g++ \
    make \
    ncat \
    && rm -rf /var/lib/apt/lists/*

# Verify versions
RUN g++ --version
RUN make --version
RUN ncat --version