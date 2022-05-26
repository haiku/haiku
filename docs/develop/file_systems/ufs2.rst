The UFS2 filesystem
===============================

While making a device for testing I have used a usb drive and formatted it to
UFS2 by using the following commands in FreeBSD. Here da0 is usb.

	gpart destroy -F /dev/da0

	gpart create -s gpt /dev/da0

	gpart add -t freebsd-ufs /dev/da0

	newfs /dev/da0p1

By running the following commands you can run the implemented code of UFS2.

Commands
--------

# Building the ufs2 shell

	jam -q "<build>ufs2_shell"

To run it, use

	jam run objects/linux/x86_64/release/tests/add-ons/kernel/file_systems/ufs2/ufs2_shell/ufs2_shell <path_to_the_image>

If you are using a usb drive then you may not be able to open it so, you just
need to add sudo in the beginning of above command and make sure that you have
not mounted the usb drive.

Alternatively you can start from an existing freebsd image, so it has some files in it:

	Download FreeBSD-12.1-RELEASE-amd64-mini-memstick.img

	diskimage register FreeBSD-12.1-RELEASE-amd64-mini-memstick.img to access the MBR style partitions inside it (on Linux probably using mount -o loop or something like that)

	dd if=/dev/disk/virtual/files/8/1 bs=8K skip=1 of=fbsd_ufstest.img to extract the filesystem from the partition (skipping the freebsd disklabel)

	Check the result: file fbsd_ufstest.img

fbsd_ufstest.img: Unix Fast File system [v2] (little-endian)

During the implementation of the project the following links were found useful.
https://github.com/freebsd/freebsd/blob/master/sys/ufs/ffs/fs.h
https://flylib.com/books/en/2.48.1/ufs2_inodes.html
