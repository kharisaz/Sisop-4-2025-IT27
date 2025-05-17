FROM ubuntu:22.04

RUN apt-get update && \
    apt-get install -y build-essential pkg-config libfuse3-dev fuse3

# buat direktori mount di container (bind‐mount di override oleh docker-compose)
RUN mkdir -p /it24_host /antink_mount /antink_logs

COPY antink.c /src/antink.c
WORKDIR /src

# Compile dengan debug symbols
RUN gcc -g antink.c -o antink `pkg-config fuse3 --cflags --libs`

# Default CMD (bisa di-override oleh docker-compose)
CMD ["/src/antink", "/it24_host", "/antink_mount"]
