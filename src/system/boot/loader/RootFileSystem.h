/*
 * Copyright 2003-2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef ROOT_FILE_SYSTEM_H
#define ROOT_FILE_SYSTEM_H


#include <boot/vfs.h>
#include <boot/partitions.h>

using namespace boot;


class RootFileSystem : public Directory {
	public:
		RootFileSystem();
		virtual ~RootFileSystem();

		virtual status_t Open(void **_cookie, int mode);
		virtual status_t Close(void *cookie);

		virtual Node* LookupDontTraverse(const char* name);

		virtual status_t GetNextEntry(void *cookie, char *nameBuffer, size_t bufferSize);
		virtual status_t GetNextNode(void *cookie, Node **_node);
		virtual status_t Rewind(void *cookie);
		virtual bool IsEmpty();

		status_t AddVolume(Directory *volume, Partition *partition);
		status_t AddLink(const char *name, Directory *target);

		status_t GetPartitionFor(Directory *volume, Partition **_partition);

	private:
		struct entry : public DoublyLinkedListLinkImpl<entry> {
			const char	*name;
			Directory	*root;
			Partition	*partition;
		};
		typedef DoublyLinkedList<entry>::Iterator EntryIterator;
		typedef DoublyLinkedList<entry> EntryList;

		EntryList fList;
		EntryList fLinks;
};

extern RootFileSystem *gRoot;

#endif	/* ROOT_FILE_SYSTEM_H */
