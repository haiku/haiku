/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/
#ifndef KERNEL_BOOT_VFS_H
#define KERNEL_BOOT_VFS_H


#include <SupportDefs.h>

#include <util/DoublyLinkedList.h>
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

		virtual status_t GetName(char *nameBuffer, size_t bufferSize) const;
		virtual int32 Type() const;
		virtual off_t Size() const;
		virtual ino_t Inode() const;

		status_t Acquire();
		status_t Release();

		DoublyLinked::Link	fLink;

	protected:
		int32		fRefCount;
};

typedef DoublyLinked::List<Node> NodeList;
typedef DoublyLinked::Iterator<Node> NodeIterator;


class Directory : public Node {
	public:
		Directory();

		virtual ssize_t ReadAt(void *cookie, off_t pos, void *buffer, size_t bufferSize);
		virtual ssize_t WriteAt(void *cookie, off_t pos, const void *buffer, size_t bufferSize);

		virtual int32 Type() const;

		virtual Node *Lookup(const char *name, bool traverseLinks) = 0;

		virtual status_t GetNextEntry(void *cookie, char *nameBuffer, size_t bufferSize) = 0;
		virtual status_t GetNextNode(void *cookie, Node **_node) = 0;
		virtual status_t Rewind(void *cookie) = 0;
		virtual bool IsEmpty() = 0;
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

/* function prototypes */

extern status_t vfs_init(stage2_args *args);
extern void register_boot_file_system(Directory *directory);
extern Directory *get_boot_file_system(stage2_args *args);
extern status_t mount_file_systems(stage2_args *args);
extern int open_node(Node *node, int mode);
extern int open_from(Directory *directory, const char *path, int mode);

extern status_t add_partitions_for(int fd, bool mountFileSystems);
extern status_t add_partitions_for(Node *device, bool mountFileSystems);

#endif	/* __cplusplus */

#endif	/* KERNEL_BOOT_VFS_H */
