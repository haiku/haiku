/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/
#ifndef ROOT_FILE_SYSTEM_H
#define ROOT_FILE_SYSTEM_H


#include <boot/vfs.h>


class RootFileSystem : public Directory {
	public:
		RootFileSystem();
		virtual ~RootFileSystem();

		virtual status_t Open(void **_cookie, int mode);
		virtual status_t Close(void *cookie);

		virtual Node *Lookup(const char *name, bool traverseLinks);

		virtual status_t GetNextEntry(void *cookie, char *nameBuffer, size_t bufferSize);
		virtual status_t GetNextNode(void *cookie, Node **_node);
		virtual status_t Rewind(void *cookie);
		virtual bool IsEmpty();

		status_t AddVolume(Directory *volume);
		status_t AddLink(const char *name, Directory *target);

	private:
		struct entry {
			DoublyLinked::Link	link;
			const char	*name;
			Directory	*root;
		};
		typedef DoublyLinked::Iterator<entry, &entry::link> EntryIterator;
		typedef DoublyLinked::List<entry, &entry::link> EntryList;

		EntryList fList;
		EntryList fLinks;
};

extern RootFileSystem *gRoot;

#endif	/* ROOT_FILE_SYSTEM_H */
