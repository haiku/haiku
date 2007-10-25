/*
 * Copyright 2002-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _STATABLE_H
#define _STATABLE_H


#include <SupportDefs.h>

#include <sys/types.h>
#include <sys/stat.h>

struct node_ref;
class BVolume;
  

class BStatable {
	public:
		virtual ~BStatable();

		virtual status_t GetStat(struct stat *st) const = 0;

		bool IsFile() const;
		bool IsDirectory() const;
		bool IsSymLink() const;

		status_t GetNodeRef(node_ref *ref) const;

		status_t GetOwner(uid_t *owner) const;
		status_t SetOwner(uid_t owner);

		status_t GetGroup(gid_t *group) const;
		status_t SetGroup(gid_t group);

		status_t GetPermissions(mode_t *perms) const;
		status_t SetPermissions(mode_t perms);

		status_t GetSize(off_t *size) const; 

		status_t GetModificationTime(time_t *mtime) const;
		status_t SetModificationTime(time_t mtime);

		status_t GetCreationTime(time_t *ctime) const;
		status_t SetCreationTime(time_t ctime);

		status_t GetAccessTime(time_t *atime) const;
		status_t SetAccessTime(time_t atime);

		status_t GetVolume(BVolume *vol) const;

	private:
		friend class BEntry;
		friend class BNode;

		virtual	void _OhSoStatable1();
		virtual	void _OhSoStatable2();
		virtual	void _OhSoStatable3();
		uint32 _reserved[4];

		virtual	status_t set_stat(struct stat &st, uint32 what) = 0;
};

#endif // _STATABLE_H
