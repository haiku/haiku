/*
 * Copyright (c) 2006-2007 Troeglazov Gerasim (3dEyes**)
 *
 * This program/include file is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program/include file is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program (in the main directory of the Linux-NTFS
 * distribution in the file COPYING); if not, write to the Free Software
 * Foundation,Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef _NTFS_H
#define _NTFS_H


#ifdef __cplusplus
extern "C" {
#endif


#include <fs_attr.h>
#include <fs_cache.h>
#include <fs_info.h>
#include <fs_interface.h>
#include <fs_info.h>
#include <fs_index.h>
#include <fs_query.h>
#include <fs_volume.h>
#include <lock.h>
#include <NodeMonitor.h>
#include <util/kernel_cpp.h>

#include "config.h"
#include "attrib.h"
#include "inode.h"
#include "volume.h"
#include "dir.h"
#include "unistr.h"
#include "layout.h"
#include "index.h"
#include "utils.h"
#include "ntfstime.h"
#include "misc.h"
#include "utils.h"
#include "bootsect.h"

#include "lock.h"
#include "ntfsdir.h"

#define  	TRACE(...) ;
//#define		TRACE dprintf
#define 	ERROR dprintf

#define 	DEV_FD(dev)	(*(int *)dev->d_private)

#define 	LOCK_VOL(vol) \
{ \
if (vol == NULL) { \
	return EINVAL; \
} \
\
LOCK((vol)->vlock); \
}

#define 	UNLOCK_VOL(vol) \
{ \
UNLOCK((vol)->vlock); \
}

typedef enum {
	NF_FreeClustersOutdate	= (1 << 0),  		// Information about amount of free clusters is outdated.
	NF_FreeMFTOutdate	= (1 << 1),  			//	Information about amount of	free MFT records is outdated.
} ntfs_state_bits;


typedef struct vnode {
	u64			vnid;
	u64			parent_vnid;
	char 		*mime;
} vnode;

typedef struct filecookie {
	int			omode;
	off_t 		last_size;
} filecookie;

typedef struct attrcookie {
	int			omode;
	ino_t		vnid;
	ntfschar*	uname;
	int			uname_len;
	uint32		type;
} attrcookie;

typedef struct attrdircookie {
	ntfs_inode*	inode;
	ntfs_attr_search_ctx *ctx;
} attrdircookie;

#define ntfs_mark_free_space_outdated(ns) (ns->state |= (NF_FreeClustersOutdate | NF_FreeMFTOutdate));



typedef struct nspace {
	ntfs_volume	*ntvol;
	char		devicePath[MAX_PATH];
	dev_t		id;
	int			free_cluster_count;
	char		volumeLabel[MAX_PATH];
	ulong 		flags;
	int 		state;
	s64 		free_clusters;
	long 		free_mft;
	BOOL 		ro;
	BOOL 		show_sys_files;
	BOOL		fake_attrib;
	BOOL 		silent;
	BOOL 		force;
	BOOL 		debug;
	BOOL 		noatime;
	BOOL 		no_detach;
	lock		vlock;
} nspace;


#ifdef __cplusplus
}
#endif


#endif

