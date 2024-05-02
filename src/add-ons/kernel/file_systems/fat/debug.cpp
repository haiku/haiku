/*
 * Copyright 1999-2001, Be Incorporated.   All Rights Reserved.
 * Copyright 2024, Haiku, Inc. All rights reserved.
 * This file may be used under the terms of the Be Sample Code License.
 */


#include "debug.h"

#include <stdlib.h>

#ifdef FS_SHELL
#include "fssh_api_wrapper.h"
#endif

#define _KERNEL

extern "C"
{
#include "sys/param.h"
#include "sys/buf.h"
#include "sys/conf.h"
#include "sys/iconv.h"
#include "sys/mount.h"
#include "sys/mutex.h"
#include "sys/namei.h"
#include "sys/vnode.h"

#include "fs/msdosfs/bootsect.h"
#include "fs/msdosfs/bpb.h"
#include "fs/msdosfs/denode.h"
#include "fs/msdosfs/direntry.h"
#include "fs/msdosfs/fat.h"
#include "fs/msdosfs/msdosfsmount.h"
}

#include "vcache.h"


static status_t
_dump_volume(const mount* bsdVolume, void (*printFunc)(const char*, ...))
{
#ifdef _KERNEL_MODE
	if (bsdVolume == NULL)
		return B_BAD_VALUE;

	msdosfsmount* fatVolume = reinterpret_cast<msdosfsmount*>(bsdVolume->mnt_data);

	cdev* dev = fatVolume->pm_dev;

	if (FAT12(fatVolume) == true)
		printFunc("FAT12 volume ");
	else if (FAT16(fatVolume))
		printFunc("FAT16 volume ");
	else if (FAT32(fatVolume))
		printFunc("FAT32 volume ");
	printFunc("@ %p\n", bsdVolume);
	printFunc("volume label [%s], file %s, device %" B_PRIdDEV ", fd %u,\n", dev->si_name,
		dev->si_device, dev->si_id, dev->si_fd);
	printFunc("mounted at %s\n", bsdVolume->mnt_stat.f_mntonname);
	printFunc("msdosfsmount flags 0x%" B_PRIx64 ", media descriptor 0x%" B_PRIx8 "\n",
		fatVolume->pm_flags, fatVolume->pm_Media);
	printFunc("%" B_PRIu32 " bytes/sector, %lu sectors/cluster\n", fatVolume->pm_BytesPerSec,
		SECTORS_PER_CLUSTER(fatVolume));
	printFunc("%" B_PRIu16 " reserved sectors, %" B_PRIu16 " FATs, %lu blocks per FAT\n",
		fatVolume->pm_ResSectors, fatVolume->pm_FATs, fatVolume->pm_FATsecs);
	if (FAT32(fatVolume) == true) {
		printFunc("fat mirroring is %s, fs info sector at block %lu, data start block %lu\n",
			fatVolume->pm_curfat == 0 ? "on" : "off", fatVolume->pm_fsinfo,
			fatVolume->pm_firstcluster);
	} else {
		printFunc("root start block %lu, %lu root blocks, data start block %lu\n",
			fatVolume->pm_rootdirblk, fatVolume->pm_rootdirsize, fatVolume->pm_firstcluster);
	}
	printFunc("%" B_PRIu32 " total blocks, %lu max cluster number, %lu free clusters\n",
		fatVolume->pm_HugeSectors, fatVolume->pm_maxcluster, fatVolume->pm_freeclustercount);
	printFunc("last fake vnid %" B_PRIdINO ", vnid cache @ (%p %p)\n", bsdVolume->vcache.cur_vnid,
		bsdVolume->vcache.by_vnid, bsdVolume->vcache.by_loc);

	dump_vcache(bsdVolume);
#endif // _KERNEL_MODE

	return B_OK;
}


int
kprintf_volume(int argc, char** argv)
{
	if ((argc == 1) || strcmp(argv[1], "--help") == 0) {
		kprintf("usage: fat <address>\n"
			"  address:  address of a FAT private volume (struct mount)\n"
			"  Use 'mounts' to list mounted volume ids, and 'mount <id>' to display a private "
			"volume address.\n");
		return 0;
	}

	mount* bsdVolume = reinterpret_cast<mount*>(strtoul(argv[1], NULL, 0));

	return _dump_volume(bsdVolume, kprintf);
}


status_t
dprintf_volume(mount* bsdVolume)
{
#ifdef _KERNEL_MODE
	return _dump_volume(bsdVolume, dprintf);
#else
	return B_OK;
#endif // !_KERNEL_MODE
}


static status_t
_dump_node(const vnode* bsdNode, void (*printFunc)(const char*, ...))
{
#ifdef _KERNEL_MODE
	if (bsdNode == NULL)
		return B_BAD_VALUE;

	denode* fatNode = reinterpret_cast<denode*>(bsdNode->v_data);

	printFunc("private node @ %p\n", bsdNode);
	printFunc("inode # %lu\n", fatNode->de_inode);
	if (bsdNode->v_type == VNON)
		printFunc("v_type VNON, ");
	else if (bsdNode->v_type == VDIR)
		printFunc("v_type VDIR, ");
	else if (bsdNode->v_type == VREG)
		printFunc("v_type VREG, ");
	if (bsdNode->v_state == VSTATE_UNINITIALIZED)
		printFunc("v_state VSTATE_UNINITIALIZED\n");
	else if (bsdNode->v_state == VSTATE_CONSTRUCTED)
		printFunc("v_state VSTATE_CONSTRUCTED\n");
	printFunc("short name %s\n", fatNode->de_Name);
	printFunc("entry cluster %lu\n", fatNode->de_dirclust);
	printFunc("entry offset %lu\n", fatNode->de_diroffset);
	printFunc("file start cluster %lu\n", fatNode->de_StartCluster);
	printFunc("size %lu bytes\n", fatNode->de_FileSize);
	printFunc("rw lock @ %p\n", &bsdNode->v_vnlock->haikuRW);
#endif // _KERNEL_MODE

	return B_OK;
}


int
kprintf_node(int argc, char** argv)
{
	if ((argc == 1) || strcmp(argv[1], "--help") == 0) {
		kprintf("usage: fat_node <address>\n"
			"  address:  address of a FAT private node (BSD struct vnode)\n"
			"  Node addresses can be found with the 'vnodes' command.\n"
			"  If the driver was built with the DEBUG flag, the command 'fat' lists node"
			"addresses\n");
		return B_OK;
	}

	vnode* bsdNode = reinterpret_cast<vnode*>(strtoul(argv[1], NULL, 0));

	return _dump_node(bsdNode, kprintf);
}


status_t
dprintf_node(vnode* bsdNode)
{
#ifdef _KERNEL_MODE
	return _dump_node(bsdNode, dprintf);
#else
	return B_OK;
#endif // !_KERNEL_MODE
}


status_t
dprintf_winentry(msdosfsmount* fatVolume, winentry* entry, const uint32* index)
{
#ifdef DEBUG
	uint8 order = entry->weCnt & ~WIN_LAST;
	mbnambuf namePart;
	mbnambuf_init(&namePart);
	win2unixfn(&namePart, entry, entry->weChksum, fatVolume);

	namePart.nb_buf[(order - 1) * 14 + namePart.nb_len] = '\0';
	char* namePtr = namePart.nb_buf + (order - 1) * 13;

	const char* lastNote;
	if ((entry->weCnt & WIN_LAST) != 0)
		lastNote = " (last one)";
	else
		lastNote = "";

	PRINT("index %" B_PRIu32 ":  winentry %u%s, checksum %u, filename part %s\n", *index, order,
		lastNote, entry->weChksum, namePtr);
#endif // DEBUG

	return B_OK;
}
