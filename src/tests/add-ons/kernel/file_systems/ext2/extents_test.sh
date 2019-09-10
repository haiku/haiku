#!/bin/sh

# This test require specially hacked fsx and fstorture binaries from here:
# https://github.com/thedrgreenthumb/fstools

TEST_DEV="/dev/disk/ata/1/master/raw"
TEST_MP="/mount"

MY_PATH=${PWD}

run_fstorture() # ${1} => block size ${2} => features list
{
	echo "Run test with bs=${1} and features ${2} ..."

	mkfs.ext3 -b ${1} ${2} -F ${TEST_DEV}
	dumpe2fs ${TEST_DEV} | grep 'Filesystem features'

	mkdir -p ${TEST_MP}
	mount -t ext2 ${TEST_DEV} ${TEST_MP}

	if [ ! -f /bin/fstorture ]; then
		echo "Can not find test binary in ${PWD}"
		exit 1
	fi

	cp /bin/fstorture ${TEST_MP}/fstorture

	mkdir ${TEST_MP}/root0 ${TEST_MP}/root1
	cd ${TEST_MP} && ./fstorture root0 root1 1 -c 1000 nosoftlinks

	cd ${MY_PATH}

	sleep 5

	unmount ${TEST_MP}
	if [ "$?" -ne "0" ]; then
		echo "Can not unmount..."
		exit 1
	fi

	e2fsck -f -n ${TEST_DEV}
	if [ "$?" -ne "0" ]; then
		echo "e2fsck fail"
		exit 1
	fi
}

run_fsx() # ${1} => block size ${2} => features list
{
	echo "Run run_fsx_combined_parallel with bs=${1} and features ${2} ..."

	mkfs.ext3 -b ${1} ${2} -F ${TEST_DEV}
	dumpe2fs ${TEST_DEV} | grep 'Filesystem features'

	mkdir -p ${TEST_MP}
	mount -t ext2 ${TEST_DEV} ${TEST_MP}

	if [ ! -f /bin/fsx ]; then
		echo "Can not find test binary in ${PWD}"
		exit 1
	fi

	cp /bin/fsx ${TEST_MP}/fsx

	cd ${TEST_MP}

	NUM_OPS=2000
	SEED=0
	./fsx -S ${SEED} -N ${NUM_OPS}                       ./TEST_FILE0 &
	./fsx -S ${SEED} -l 5234123 -o 5156343 -N ${NUM_OPS} ./TEST_FILE1 &
	./fsx -S ${SEED} -l 2311244 -o 2311200 -N ${NUM_OPS} ./TEST_FILE2 &
	./fsx -S ${SEED} -l 8773121 -o 863672  -N ${NUM_OPS} ./TEST_FILE3 &
	./fsx -S ${SEED} -l 234521 -o 234521   -N ${NUM_OPS} ./TEST_FILE4 &
	./fsx -S ${SEED} -l 454321 -o 33       -N ${NUM_OPS} ./TEST_FILE5 &
	./fsx -S ${SEED} -l 7234125 -o 7876728 -N ${NUM_OPS} ./TEST_FILE6 &
	./fsx -S ${SEED} -l 5646463 -o 4626734 -N ${NUM_OPS} ./TEST_FILE7 &

	for job in `jobs -p`
	do
		wait $job
	done

	cd ${MY_PATH}

	sleep 5

	unmount ${TEST_MP}
	if [ "$?" -ne "0" ]; then
		echo "Can not unmount..."
		exit 1
	fi

	e2fsck -f -n ${TEST_DEV}
	if [ "$?" -ne "0" ]; then
		echo "e2fsck fail"
		exit 1
	fi
}

# main()
pkgman install -y cmd:fstorture cmd:fsx cmd:e2fsck

FEATURES="-O huge_file -O dir_nlink -O extent"
run_fsx "1024" "$FEATURES"
run_fstorture "1024" "$FEATURES"

run_fsx "2048" "$FEATURES"
run_fstorture "2048" "$FEATURES"

run_fsx "4096" "$FEATURES"
run_fstorture "4096" "$FEATURES"

#run_fsx "64k" "-O huge_file -O dir_nlink -O extent"
#run_fstorture "64k" "-O huge_file -O dir_nlink -O extent"

FEATURES="$FEATURES -O 64bit"

run_fsx "1024" "$FEATURES"
run_fstorture "1024" "$FEATURES"

run_fsx "2048" "$FEATURES"
run_fstorture "2048" "$FEATURES"

run_fsx "4096" "$FEATURES"
run_fstorture "4096" "$FEATURES"

FEATURES="$FEATURES -O metadata_csum"

run_fsx "1024" "$FEATURES"
run_fstorture "1024" "$FEATURES"

run_fsx "2048" "$FEATURES"
run_fstorture "2048" "$FEATURES"

run_fsx "4096" "$FEATURES"
run_fstorture "4096" "$FEATURES"

echo PASSED

