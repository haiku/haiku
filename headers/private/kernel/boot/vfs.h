/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/
#ifndef KERNEL_BOOT_VFS_H
#define KERNEL_BOOT_VFS_H


#include <SupportDefs.h>

#include <list.h>
#include <boot/stage2_args.h>


#ifdef __cplusplus

/** This is the base class for all VFS nodes */

class Node {
	public:
		Node();
		virtual ~Node();

		virtual status_t Open(void **_cookie, int mode);
		virtual status_t Close(void *cookie);

		virtual ssize_t ReadAt(void *cookie, off_t pos, void *buffer, size_t bufferSize) = 0;
		virtual ssize_t WriteAt(void *cookie, off_t pos, const void *buffer, size_t bufferSize) = 0;

	protected:
		list_link	fLink;
		int32		fRefCount;
};


/** The console based nodes don't need cookies for I/O, they
 *	also don't support to change the stream position.
 *	Live is simple in the boot loader :-)
 */

class ConsoleNode : public Node {
	public:
		ConsoleNode();

		virtual ssize_t Read(void *buffer, size_t bufferSize);
		virtual ssize_t Write(const void *buffer, size_t bufferSize);
};


extern "C" {
#endif	/* __cplusplus */

extern status_t vfs_init(stage2_args *args);
extern status_t mount_boot_file_systems();
extern status_t add_partitions_for(int fd);

#ifdef __cplusplus
}
#endif

#endif	/* KERNEL_BOOT_VFS_H */
