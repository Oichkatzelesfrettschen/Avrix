FROM ubuntu:24.04

RUN apt-get update && \
    apt-get install -y software-properties-common apt-transport-https ca-certificates gnupg && \
    echo "deb [arch=amd64 signed-by=/usr/share/keyrings/debian-archive-keyring.gpg] http://deb.debian.org/debian sid main" > /etc/apt/sources.list.d/debian-sid-avr.list && \
    echo "Package: gcc-avr avr-libc binutils-avr\nPin: release o=Debian,a=sid\nPin-Priority: 100" > /etc/apt/preferences.d/90avr-cross && \
    apt-get update && \
    apt-get install -y gcc-avr avr-libc binutils-avr avrdude gdb-avr simavr \
                       qemu-system-misc meson ninja-build doxygen python3-sphinx \
                       python3-pip graphviz git nodejs npm cloc cscope exuberant-ctags cppcheck && \
    pip3 install --no-cache-dir breathe exhale sphinx-rtd-theme && \
    npm install -g prettier && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /workspace
