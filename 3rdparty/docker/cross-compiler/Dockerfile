FROM debian:bookworm-slim

# docker build --no-cache --tag ghcr.io/haiku/cross-compiler:x86_64 .
# docker push ghcr.io/haiku/cross-compiler:x86_64

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
  libzstd-dev \
  make \
  nasm \
  ninja-build \
  python3 \
  texinfo \
  vim \
  wget \
  xz-utils \
  zlib1g-dev; \
  if [ $(uname -m) = "x86_64" ]; then apt-get install -y g++-multilib; fi

# source revision to build
ARG BUILDTOOLS_REV=master
ARG HAIKU_REV=master
# architectures to build
ARG ARCHITECTURE=x86_64
ARG SECONDARY_ARCHITECTURE=

# Build Haiku cross-compiler toolchain, and haiku package tool
WORKDIR /tmp
COPY build-toolchain.sh /tmp/
RUN chmod 755 /tmp/build-toolchain.sh
RUN /tmp/build-toolchain.sh $BUILDTOOLS_REV $HAIKU_REV $ARCHITECTURE $SECONDARY_ARCHITECTURE

WORKDIR /
