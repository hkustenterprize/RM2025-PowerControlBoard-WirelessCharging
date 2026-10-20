FROM ubuntu:22.04
LABEL name="embedded-dev-env" \
      version="1.0"

ENV DEBIAN_FRONTEND=noninteractive \
    USER=root \
    HOME=/root \
    WORKDIR=/workspace

WORKDIR ${WORKDIR}

RUN sed -i 's@http://[a-z.]*ubuntu.com@http://mirrors.ustc.edu.cn@g' /etc/apt/sources.list && \
    apt-get update && \
    apt-get install -y --no-install-recommends \
        build-essential \
        cmake \
        ninja-build \
        libnewlib-arm-none-eabi \
        gcc-arm-none-eabi && \
    rm -rf /var/lib/apt/lists/*

COPY . .

CMD ["/bin/bash"]