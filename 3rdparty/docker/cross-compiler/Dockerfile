FROM debian:buster-slim

# docker build --no-cache --tag docker.io/haiku/cross-compiler:x86_64 .
# docker push docker.io/haiku/cross-compiler:x86_64

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
  ninja-build \
  python \
  texinfo \
  vim \
  wget \
  xz-utils \
  zlib1g-dev

# architectures to build
ARG ARCHITECTURE=x86_64
ARG SECONDARY_ARCHITECTURE=

# Build Haiku cross-compiler toolchain, and haiku package tool
WORKDIR /tmp
COPY build-toolchain.sh /tmp/
RUN chmod 755 /tmp/build-toolchain.sh
RUN /tmp/build-toolchain.sh $ARCHITECTURE $SECONDARY_ARCHITECTURE

WORKDIR /
