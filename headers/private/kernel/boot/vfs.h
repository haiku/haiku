/*
 * Copyright 2003-2005, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_BOOT_VFS_H
#define KERNEL_BOOT_VFS_H


#include <SupportDefs.h>

#include <util/DoublyLinkedList.h>
#include <boot/stage2_args.h>


#ifdef __cplusplus

struct file_map_run;

/** This is the base class for all VFS nodes */

class Node : public DoublyLinkedListLinkImpl<Node> {
	public:
		Node();
		virtual ~Node();

		virtual status_t Open(void **_cookie, int mode);
		virtual status_t Close(void *cookie);

		virtual ssize_t ReadAt(void *cookie, off_t pos, void *buffer, size_t bufferSize) = 0;
		virtual ssize_t WriteAt(void *cookie, off_t pos, const void *buffer, size_t bufferSize) = 0;

		virtual status_t GetName(char *nameBuffer, size_t bufferSize) const;
		virtual status_t GetFileMap(struct file_map_run *runs, int32 *count);
		virtual int32 Type() const;
		virtual off_t Size() const;
		virtual ino_t Inode() const;

		status_t Acquire();
		status_t Release();

	protected:
		int32		fRefCount;
};

typedef DoublyLinkedList<Node> NodeList;
typedef NodeList::Iterator NodeIterator;


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

		virtual status_t CreateFile(const char *name, mode_t permissions,
			Node **_node);
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


class MemoryDisk : public Node {
	public:
		MemoryDisk(const uint8* data, size_t size, const char* name);

		virtual ssize_t ReadAt(void* cookie, off_t pos, void* buffer,
			size_t bufferSize);
		virtual ssize_t WriteAt(void* cookie, off_t pos, const void* buffer,
			size_t bufferSize);

		virtual off_t Size() const;
		virtual status_t GetName(char *nameBuffer, size_t bufferSize) const;

	private:
		const uint8*	fData;
		size_t			fSize;
		char			fName[64];
};


class BootVolume {
public:
								BootVolume();
								~BootVolume();

		status_t				SetTo(Directory* rootDirectory);
		void					Unset();

		bool					IsValid() const
									{ return fRootDirectory != NULL; }

		Directory*				RootDirectory() const
									{ return fRootDirectory; }
		Directory*				SystemDirectory() const
									{ return fSystemDirectory; }
		bool					IsPackaged() const
									{ return fPackaged; }

private:
			Directory*			fRootDirectory;
				// root directory of the volume
			Directory*			fSystemDirectory;
				// "system" directory of the volume; if packaged the root
				// directory of the mounted packagefs
			bool				fPackaged;
				// indicates whether the boot volume's system is packaged
};


/* function prototypes */

extern status_t vfs_init(stage2_args *args);
extern status_t register_boot_file_system(Directory *directory);
extern status_t get_boot_file_system(stage2_args* args,
			BootVolume& _bootVolume);
extern status_t mount_file_systems(stage2_args *args);
extern int open_node(Node *node, int mode);
extern int open_from(Directory *directory, const char *path, int mode,
			mode_t permissions = 0);

extern Node *get_node_from(int fd);

extern status_t add_partitions_for(int fd, bool mountFileSystems, bool isBootDevice = false);
extern status_t add_partitions_for(Node *device, bool mountFileSystems, bool isBootDevice = false);

#endif	/* __cplusplus */

#endif	/* KERNEL_BOOT_VFS_H */
