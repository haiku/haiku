/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/
#ifndef PARTITION_H
#define PARTITION_H


#include <boot/vfs.h>
#include <ddm_modules.h>

#include <stddef.h>


class Partition : public partition_data, public Node {
	public:
		Partition(int deviceFD);
		virtual ~Partition();

		virtual ssize_t ReadAt(void *cookie, off_t offset, void *buffer, size_t bufferSize);
		virtual ssize_t WriteAt(void *cookie, off_t offset, const void *buffer, size_t bufferSize);

		Partition *AddChild();
		status_t Scan();

		Partition *Parent() const { return fParent; }
		bool IsFileSystem() const { return fIsFileSystem; }

		static size_t LinkOffset() { return sizeof(partition_data); }

	private:
		void SetParent(Partition *parent) { fParent = parent; }

		int			fFD;
		list		fChildren;
		Partition	*fParent;
		bool		fIsFileSystem;
};

#endif	/* PARTITION_H */
