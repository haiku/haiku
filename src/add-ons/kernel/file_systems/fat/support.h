/*
 * Copyright 1999-2001, Be Incorporated.   All Rights Reserved.
 * Copyright 2001-2020, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2024, Haiku, Inc. All rights reserved.
 * This file may be used under the terms of the Be Sample Code License.
 */

/*-
 * SPDX-License-Identifier: BSD-4-Clause
 *
 * Copyright (C) 1994, 1995, 1997 Wolfgang Solfrank.
 * Copyright (C) 1994, 1995, 1997 TooLs GmbH.
 * All rights reserved.
 * Original code by Paul Popelka (paulp@uts.amdahl.com) (see below).
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*-
 * Written by Paul Popelka (paulp@uts.amdahl.com)
 *
 * You can do anything you want with this software, just don't say you wrote
 * it, and don't remove this notice.
 *
 * This software is provided "as is".
 *
 * The author supplies this software to be publicly redistributed on the
 * understanding that the author is not responsible for the correct
 * functioning of this software in any circumstances and is not liable for
 * any damages caused by this software.
 *
 * October 1992
 */
#ifndef FAT_SUPPORT_H
#define FAT_SUPPORT_H


// Support functions for C++ driver code.

#ifdef FS_SHELL
#include "fssh_api_wrapper.h"
#else
#include <lock.h>
#endif // FS_SHELL

#define _KERNEL
extern "C"
{
#include "sys/param.h"	
#include "sys/buf.h"
#include "sys/conf.h"
#include "sys/iconv.h"
#include "sys/mount.h"
#include "sys/namei.h"
#include "sys/vnode.h"

#include "fs/msdosfs/bootsect.h"
#include "fs/msdosfs/bpb.h"
#include "fs/msdosfs/denode.h"
#include "fs/msdosfs/direntry.h"
#include "fs/msdosfs/fat.h"
#include "fs/msdosfs/msdosfsmount.h"
}

//**************************************
// File names and volume labels

// size of buffer needed to store a FAT short filename as a null-terminated string
#define SHORTNAME_CSTRING 12
// size of the array that holds a short filename on disk / strlen of a null-terminated name
#define SHORTNAME_LENGTH 11
#define LABEL_CSTRING 12
#define LABEL_LENGTH 11

// legal characters in a volume label
const char sAcceptable[] = "!#$%&'()-0123456789@ABCDEFGHIJKLMNOPQRSTUVWXYZ^_`{}~";
// characters not permitted in a file name
const char sIllegal[] = "\\/:*?\"<>|";
// characters not permitted in a volume label
extern const char* LABEL_ILLEGAL;

// C++ struct used to simplify handling of the FreeBSD C struct componentname
struct ComponentName {
	ComponentName(u_int64_t flags, struct ucred* cred, enum nameiop nameiop, int lkflags,
		const char* nameptr);
	~ComponentName();

	componentname* Data();

	componentname fData;
};

bool is_filename_legal(const char* name);
bool is_shortname_legal(const u_char* name);
void sanitize_label(char* name);
status_t read_label(const msdosfsmount* volume, int fd, const uint8* buffer, char* label);

//**************************************
// Bootsector and fsinfo sector

// This is analagous to byte_bpb33 etc. defined in bpb.h, but it includes only those fields that
// are in the same position and have the same size for all 3 possible FAT BPB formats.
struct universal_byte_bpb {
	int8_t bpbBytesPerSec[2]; /* bytes per sector */
	int8_t bpbSecPerClust; /* sectors per cluster */
	int8_t bpbResSectors[2]; /* number of reserved sectors */
	int8_t bpbFATs; /* number of FATs */
	int8_t bpbRootDirEnts[2]; /* number of root directory entries */
	int8_t bpbSectors[2]; /* total number of sectors */
	int8_t bpbMedia; /* media descriptor */
	int8_t bpbFATsecs[2]; /* number of sectors per FAT */
	int8_t bpbSecPerTrack[2]; /* sectors per track */
	int8_t bpbHeads[2]; /* number of heads */
};

status_t check_bootsector(const uint8* bootsector, FatType& _type, bool& _dos33);
status_t parse_bpb(msdosfsmount* volume, const bootsector* bootsector, bool dos33);
void fix_zip(const byte_bpb50* bpb, msdosfsmount* volume);
status_t read_fsinfo(msdosfsmount* volume, const vnode* devNode);
status_t write_fsinfo(msdosfsmount* volume);

//**************************************
// FAT and data clusters

// identifies a sector in terms of the data cluster it falls in;
// used when iterating through sectors in a file
struct csi {
	msdosfsmount* fatVolume;
	uint32 cluster;
		// file-system-relative data cluster
	uint32 sector;
		// cluster-relative sector index
};

status_t check_fat(const msdosfsmount* volume);
uint32 get_nth_fat_entry(msdosfsmount* fatVolume, uint32 cluster, uint32 n);
status_t init_csi(msdosfsmount* fatVolume, uint32 cluster, uint32 sector, struct csi* csi);
status_t validate_cs(msdosfsmount* fatVolume, uint32 cluster, uint32 sector);
off_t fs_sector(struct csi* csi);
status_t iter_csi(struct csi* csi, int sectors);

/*! Return the cluster number of the first cluster of the root directory.

*/
inline u_long
root_start_cluster(const msdosfsmount* vol)
{
	u_long result;

	if (FAT32(vol))
		result = vol->pm_rootdirblk;
	else
		result = MSDOSFSROOT;

	return result;
}


//**************************************
// directory entries

struct emptyDir {
	struct direntry dot;
	struct direntry dotdot;
};

extern struct emptyDir gDirTemplate;

//**************************************
// nodes

// Similar to class Vnode in BFS, except that get_vnode() needs to be called separately,
// before constructing this.
struct NodePutter {
	NodePutter();
	NodePutter(vnode* node);
	~NodePutter();

	void SetTo(vnode* node);
	void Put();

	vnode* fNode;
};

status_t get_location(mount* bsdVolume, ino_t vnid, u_long* dirclust, u_long* diroffset);
status_t assign_inode_and_get(mount* bsdVolume, daddr_t cluster, u_long offset, vnode** bsdNode);


inline ino_t
root_inode(msdosfsmount* volume)
{
	u_long dirClust = FAT32(volume) == true ? volume->pm_rootdirblk : MSDOSFSROOT;
	return DETOI(volume, dirClust, MSDOSFSROOT_OFS);
}


//**************************************
// times

void timestamp_local(timespec* tsp);
void local_to_GMT(const timespec* tspLocal, timespec* tspGMT);

//**************************************
// IO

status_t slist_insert_buf(vnode* deviceNode, size_t size);
status_t fill_gap_with_zeros(vnode* bsdNode, off_t pos, off_t newSize);
status_t sync_clusters(vnode* bsdNode);

//**************************************
// access

status_t check_access_permissions_internal(int accessMode, mode_t mode, gid_t nodeGroupID,
	uid_t nodeUserID);
void mode_bits(const vnode* bsdNode, mode_t* mode);

//**************************************
// MIME

status_t set_mime_type(vnode* bsdNode, bool update);

//**************************************
// libiconv

status_t iconv_init(msdosfsmount* fatVolume, const char* oemPreference);


#endif // FAT_SUPPORT_H
