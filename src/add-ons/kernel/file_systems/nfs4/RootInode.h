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
		status_t			ReadInfo(struct fs_info* info);

		bool				ProbeMigration();
		status_t			GetLocations(AttrValue** attr);

};


#endif	// ROOTINODE_H

