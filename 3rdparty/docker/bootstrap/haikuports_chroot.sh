#!/bin/bash

if [ $# -lt 1 ]; then
	echo "usage: $0 <recipe> [sysroot-stage number]"
	echo "Enters a haikuporter bootstrap chroot."
	echo "Optionally, specify the sysroot-stage (default 1)"
	exit 1
fi

STAGE="stage1"
if [ $# -eq 2 ]; then
	STAGE=stage$2
fi

export GENERATED=$WORKPATH/generated.$TARGET_ARCH
export LD_LIBRARY_PATH=$GENERATED/objects/linux/lib/:$LD_LIBRARY_PATH

# A hack to fix haikuport chroot not liking Debian's fancy PS1
export PS1='\u@\h:\w\$'

echo "Entering chroot with $STAGE chroot..."

$WORKPATH/src/haikuporter/haikuporter \
	--config=$GENERATED/objects/haiku/$TARGET_ARCH/packaging/repositories/HaikuPortsCross-build/haikuports.conf \
	--cross-devel-package $GENERATED/objects/haiku/$TARGET_ARCH/packaging/packages/haiku_cross_devel_sysroot_${STAGE}_$TARGET_ARCH.hpkg \
	--enter-chroot $1
