/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */
#ifndef ROOTINODE_H
#define ROOTINODE_H


#include <fs_info.h>

#include "Inode.h"


class RootInode : public Inode {
public:
								RootInode();
								~RootInode();

	virtual	const char*			Name() const;
	inline	void				SetName(const char* name);

			status_t			ReadInfo(struct fs_info* info);
	inline	void				MakeInfoInvalid();

	inline	uint32				IOSize();

			bool				ProbeMigration();
			status_t			GetLocations(AttrValue** attr);

private:
			struct fs_info		fInfoCache;
			mutex				fInfoCacheLock;
			time_t				fInfoCacheExpire;

			const char*			fName;

			uint32				fIOSize;

			status_t			_UpdateInfo(bool force = false);

};


inline void
RootInode::MakeInfoInvalid()
{
	fInfoCacheExpire = 0;
}


inline uint32
RootInode::IOSize()
{
	if (fIOSize == 0)
		_UpdateInfo(true);
	return fIOSize;
}


inline void
RootInode::SetName(const char* name)
{
	free(const_cast<char*>(fName));
	fName = strdup(name);
}


#endif	// ROOTINODE_H

