FROM debian:bookworm-slim

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    bochs \
    bochs-sdl \
    bochs-term \
    bochs-x \
    busybox-static \
    ca-certificates \
    curl \
    cpio \
    isolinux \
    mtools \
    syslinux-common \
    xauth \
    xorriso \
    xvfb \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /work

COPY . /work

CMD ["/bin/bash"]
