#!/bin/bash

if [ $# -ne 1 ]; then
	echo "usage: $0 <recipe>"
	exit 1
fi

export GENERATED=$WORKPATH/generated.$TARGET_ARCH
export LD_LIBRARY_PATH=$GENERATED/objects/linux/lib/:$LD_LIBRARY_PATH

# A hack to fix haikuport chroot not liking Debian's fancy PS1
export PS1='\u@\h:\w\$'

$WORKPATH/src/haikuporter/haikuporter \
	--config=$GENERATED/objects/haiku/$TARGET_ARCH/packaging/repositories/HaikuPortsCross-build/haikuports.conf \
	--cross-devel-package $GENERATED/objects/haiku/$TARGET_ARCH/packaging/packages/haiku_cross_devel_sysroot_stage1_$TARGET_ARCH.hpkg \
	--enter-chroot $1
