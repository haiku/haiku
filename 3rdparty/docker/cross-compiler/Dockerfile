FROM ubuntu:16.04

RUN apt-get update && apt-get install -y --no-install-recommends \
  autoconf \
  automake \
  bison \
  bzip2 \
  ca-certificates \
  cmake \
  curl \
  file \
  flex \
  g++ \
  gawk \
  git \
  libcurl4-openssl-dev \
  libssl-dev \
  make \
  nasm \
  texinfo \
  wget \
  xz-utils \
  zlib1g-dev

# architectures to build
ARG ARCHITECTURE=x86_64
ARG SECONDARY_ARCHITECTURE=

# Build Haiku cross-compiler toolchain, and haiku package tool
WORKDIR /tmp
COPY build-toolchain.sh /tmp/
RUN /tmp/build-toolchain.sh $ARCHITECTURE $SECONDARY_ARCHITECTURE

WORKDIR /
