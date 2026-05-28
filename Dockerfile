FROM arm64v8/ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    bison \
    busybox-static \
    ca-certificates \
    curl \
    cpio \
    flex \
    bochs \
    bochs-x \
    bochsbios \
    libtool \
    m4 \
    pkg-config \
    grub-common \
    g++-riscv64-linux-gnu \
    gcc-riscv64-linux-gnu \
    gcc-x86-64-linux-gnu \
    isolinux \
    mtools \
    libc6-dev-amd64-cross \
    linux-libc-dev-amd64-cross \
    qemu-user-static \
    squashfs-tools \
    syslinux-common \
    vgabios \
    xauth \
    xvfb \
    xorriso \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /work

COPY . /work

RUN cd /work/bochs-prepoly-src/bochs && \
    ./configure --with-nogui --enable-x86-64 --disable-docbook --disable-debugger --disable-plugins && \
    make -j2 && \
    cp bochs /usr/local/bin/bochs-poly

CMD ["/bin/bash"]
