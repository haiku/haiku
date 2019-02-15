FROM debian

ENV DEBIAN_FRONTEND="noninteractive"

RUN apt-get update

# Base Haiku requirements
RUN apt-get install -y git nasm autoconf automake texinfo flex \
	bison gawk build-essential unzip wget zip less zlib1g-dev \
	libcurl4-openssl-dev genisoimage libtool mtools gcc-multilib \
	u-boot-tools util-linux device-tree-compiler bc

# GCC Requirements
RUN apt-get install -y python
# zlib Requirements
RUN apt-get install -y cmake
# ICU Requirements
RUN apt-get install -y pkg-config

ENV GIT_BUILDTOOLS="https://git.haiku-os.org/buildtools"
ENV GIT_HAIKU="https://git.haiku-os.org/haiku"
ENV GIT_HAIKUPORTER="https://github.com/haikuports/haikuporter.git"
ENV GIT_HAIKUPORTS="https://github.com/haikuports/haikuports.git"
ENV GIT_HAIKUPORTS_CROSS="https://github.com/haikuports/haikuports.cross.git"

ADD prep.sh /usr/local/bin/prep
ADD crosstools.sh /usr/local/bin/crosstools
ADD bootstrap.sh /usr/local/bin/bootstrap
ADD haikuports_chroot.sh /usr/local/bin/haikuports_chroot
ADD haikuports_build.sh /usr/local/bin/haikuports_build

ENV WORKPATH="/work"
ENV PATH="$PATH:$WORKPATH/bin"

WORKDIR "/work"
VOLUME ["/work"]

ENTRYPOINT ["/bin/bash", "-c"]
