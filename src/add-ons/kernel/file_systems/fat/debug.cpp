/*
 * Copyright 1999-2001, Be Incorporated.   All Rights Reserved.
 * Copyright 2024-2026, Haiku, Inc. All rights reserved.
 * This file may be used under the terms of the Be Sample Code License.
 */

#include "debug.h"

#ifndef FS_SHELL
#include <stdlib.h>
#include <util/AutoLock.h>
#include <util/Vector.h>
#endif

#define _KERNEL

extern "C" {
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

#include "encodings.h"
#include "support.h"
#include "vcache.h"


static void
print_byte(uint8 c, void (*printFunc)(const char*, ...))
{
	printFunc("%c", ((c >= ' ') && (c <= '~')) ? c : '.');
}


void
_dump_bytes(const uint8* buffer, uint32 count, void (*printFunc)(const char*, ...))
{
	uint32 i, j, k;
	for (i = 0; i < 0x10; i++)
		printFunc(" %" B_PRIu32 " ", i);

	printFunc("\n");
	for (i = 0; i < count; i += 0x10) {
		j = (i + 0x10 > count) ? count - i : 0x10;
		for (k = i; k < i + j; k++)
			printFunc("%2.2X ", buffer[k]);
		for (; k < i + 0x10; k++)
			printFunc("   ");
		printFunc(" ");
		for (k = i; k < i + j; k++)
			print_byte(buffer[k], printFunc);
		printFunc("\n");
	}
}


void
dprintf_bytes(const uint8* buffer, uint32 count)
{
	_dump_bytes(buffer, count, dprintf);
	return;
}


static status_t
_dump_volume(const mount* bsdVolume, void (*printFunc)(const char*, ...))
{
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


/*!	Print volume information to the syslog.
	@pre \a bsdVolume is locked.
*/
status_t
dprintf_volume(mount* bsdVolume)
{
	return _dump_volume(bsdVolume, dprintf);
}


static status_t
_dump_node(const vnode* bsdNode, void (*printFunc)(const char*, ...))
{
	if (bsdNode == NULL)
		return B_BAD_VALUE;

	denode* fatNode = reinterpret_cast<denode*>(bsdNode->v_data);

	printFunc("private node @ %p\n", bsdNode);
	printFunc("inode # %lu\n", fatNode->de_inode);
	if (bsdNode->v_vflag == VV_ROOT)
		printFunc("v_vflag VV_ROOT, ");
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


/*!	Print volume information to the syslog.
	@pre \a bsdNode is at least read-locked.
*/
status_t
dprintf_node(vnode* bsdNode)
{
	return _dump_node(bsdNode, dprintf);
}


static status_t
_dump_direntry(direntry* entry, uint32 index, void (*printFunc)(const char*, ...))
{
	if (entry->deName[0] == SLOT_EMPTY) {
		printFunc("Entry %" B_PRIu32 ": free entry (no further entries)\n", index);
	} else if (entry->deName[0] == SLOT_DELETED) {
		printFunc("Entry %" B_PRIu32 ": free entry\n", index);
	} else {
		struct msdosfsmount dummyVolume;
		dummyVolume.pm_flags = MSDOSFS_CP850;
			// translate the short name from code page 850
		u_char utf8FromCp850[24];
			// 11 characters, at most 2 bytes per character, plus period if file has an extension,
			// plus terminating NULL
		dos2unixfn(entry->deName, utf8FromCp850, entry->deLowerCase, &dummyVolume);

		dummyVolume.pm_flags = MSDOSFS_SJIS;
			// translate the short name from SJIS
		u_char utf8FromSjis[35];
			// at most 3 bytes per character + period + NULL
		dos2unixfn(entry->deName, utf8FromSjis, entry->deLowerCase, &dummyVolume);

		char rawNameString[34];
		char* rawNamePtr = rawNameString;
		for (int i = 0; i < 11; ++i, rawNamePtr += 3)
			sprintf(rawNamePtr, "%02hX ", entry->deName[i]);
		rawNameString[32] = 0;
			// cut off the last trailing space
		if ((entry->deAttributes & ATTR_VOLUME) != 0) {
			printFunc("Entry %" B_PRIu32 ": volume label [%s] (CP850) / [%s] (SJIS) / [%s] (raw)\n",
				index, utf8FromCp850, utf8FromSjis, rawNameString);
		} else {
			uint16 crDateHostEndian = B_LENDIAN_TO_HOST_INT16(*(uint16*)entry->deCDate);
			uint32 crDay = (crDateHostEndian & DD_DAY_MASK) >> DD_DAY_SHIFT;
			uint32 crMonth = (crDateHostEndian & DD_MONTH_MASK) >> DD_MONTH_SHIFT;
			uint32 crYear = ((crDateHostEndian & DD_YEAR_MASK) >> DD_YEAR_SHIFT) + 1980;

			uint16 crTimeHostEndian = B_LENDIAN_TO_HOST_INT16(*(uint16*)entry->deCTime);
			uint32 crHour = (crTimeHostEndian & DT_HOURS_MASK) >> DT_HOURS_SHIFT;
			uint32 crMin = (crTimeHostEndian & DT_MINUTES_MASK) >> DT_MINUTES_SHIFT;
			uint32 crSec = ((crTimeHostEndian & DT_2SECONDS_MASK) >> DT_2SECONDS_SHIFT) * 2;

			printFunc("Entry %" B_PRIu32 ": direntry [%s] (CP850) / [%s] (SJIS) / [%s] (raw)\n"
					  "\tattributes 0x%" B_PRIx8 ", case flags 0x%" B_PRIx8
					  ", created at %02" B_PRIu32 ":%02" B_PRIu32 ":%02" B_PRIu32
					  " GMT on %02" B_PRIu32 "-%02" B_PRIu32 "-%" B_PRIu32 " (mm-dd-yyyy)\n",
				index, utf8FromCp850, utf8FromSjis, rawNameString, entry->deAttributes,
				entry->deLowerCase, crHour, crMin, crSec, crMonth, crDay, crYear);
		}
	}

	return B_OK;
}


/*!	Print direntry information to the syslog.
	@pre The directory node is at least read-locked.
*/
status_t
dprintf_direntry(direntry* entry, uint32 index)
{
	return _dump_direntry(entry, index, dprintf);
}


static int
_dump_winentry(const winentry* entry, uint32 index, int chkSumIn, uint16& unpaired,
	void (*printFunc)(const char*, ...))
{
	mbnambuf namePart;
	mbnambuf_init(&namePart);
	uint16 unpairedIn = unpaired;

	// get a UTF-8 translation of the name part to print alongside the UTF-16 units
	int chkSumOut = winentry_to_utf8(&namePart, entry, entry->weChksum, &unpaired);
		// normally the chkSum argument depends on the previous entry and is used to verify
		// continuity between entries, but here we just pass the entry's own checksum so
		// we can view the UTF-8 conversion even if there is a continuity problem

	char* namePtr = NULL;
	uint8 order = entry->weCnt & ~WIN_LAST;
	if (chkSumOut != -1) {
		namePart.nb_buf[(order - 1) * WIN_CHARS + namePart.nb_len] = '\0';
		namePtr = namePart.nb_buf + (order - 1) * WIN_CHARS;
	}

	const char* lastNote;
	if ((entry->weCnt & WIN_LAST) != 0)
		lastNote = " (last one)";
	else
		lastNote = "";
	printFunc("Entry %" B_PRIu32 ": winentry %u%s, checksum %u\n", index, order, lastNote,
		entry->weChksum, namePtr);

	if ((chkSumIn == -1 && (entry->weCnt & WIN_LAST) == 0)
		|| (chkSumIn >= 0 && chkSumIn != chkSumOut)) {
		printFunc("\tWarning: checksum %d does not match checksum from previous entry %d\n",
			chkSumOut, chkSumIn);
	} else if (chkSumIn >= 0 && (entry->weCnt & WIN_LAST) != 0) {
		printFunc("\tWarning: entry flagged as last, but previous entry was a winentry\n");
	}

	printFunc("\twePart1: ");
	for (int i = 0; i < WEPART1_BYTES; ++i)
		printFunc("%02hX ", entry->wePart1[i]);
	printFunc("\n\twePart2: ");
	for (int i = 0; i < WEPART2_BYTES; ++i)
		printFunc("%02hX ", entry->wePart2[i]);
	printFunc("\n\twePart3: ");
	for (int i = 0; i < WEPART3_BYTES; ++i)
		printFunc("%02hX ", entry->wePart3[i]);

	if (chkSumOut != -1)
		printFunc("\n\tUTF-8 conversion: %s\n", namePtr);
	else
		printFunc("\n\twinentry_to_utf8 failed, can't display UTF-8 string\n");

	uint16 firstUnit = B_LENDIAN_TO_HOST_INT16(*(uint16*)entry->wePart1);
	if (is_low_surrogate(firstUnit)) {
		if (unpaired == firstUnit) {
			printFunc("\tNote: lone low surrogate %" B_PRIX16 " needs to be carried to next "
					  "entry\n",
				firstUnit);
		} else if (unpaired == 0) {
			printFunc("\tWarning: lone low surrogate %" B_PRIX16 " was not identified by "
					  "winentry_to_utf8\n",
				firstUnit);
		} else if (unpaired != firstUnit) {
			printFunc("\tWarning: lone low surrogate %" B_PRIX16 " but winentry_to_utf8 "
					  "identified it as %" B_PRIX16 "\n",
				firstUnit, unpaired);
		}
	}

	uint16 lastUnit = B_LENDIAN_TO_HOST_INT16(*(uint16*)(entry->wePart3 + 2));
	if (is_high_surrogate(lastUnit)) {
		if (unpairedIn != 0) {
			printFunc("\tNote: lone high surrogate %" B_PRIX16 " combined with "
					  "low surrogate %" B_PRIX16 " from last entry\n",
				lastUnit, unpairedIn);
		} else {
			printFunc("\tWarning: lone high surrogate %" B_PRIX16 " but no low "
					  "surrogate was carried over\n",
				lastUnit);
		}
	} else if (unpairedIn != 0) {
		printFunc("\tWarning: low surrogate was carried over but there is no high surrogate in "
				  "the present entry\n");
	}

	return chkSumOut;
}


/*!	Print winentry information to the syslog.
	@param chkSum The checksum carried over from the last entry. If not available, pass -2.
	@param unpaired The UTF-16 surrogate carried over from the last entry, or zero. It will be used
	to help print the UTF-8 conversion of the long file name part in this entry.
	@pre The directory node is at least read-locked.
	@post The function does not output the new unpaired or chkSum values. If the client
	needs those, it should call winentry_to_utf8 directly.
*/
status_t
dprintf_winentry(struct winentry* entry, uint32 index, int chkSum, uint16 unpaired)
{
	_dump_winentry(entry, index, chkSum, unpaired, dprintf);

	return B_OK;
}


static status_t
_dump_fat_dir(mount* bsdVolume, const vnode* bsdNode, size_t& count,
	void (*printFunc)(const char*, ...))
{
	denode* fatNode = reinterpret_cast<denode*>(bsdNode->v_data);
	count = 0;

	if ((fatNode->de_Attributes & ATTR_DIRECTORY) == 0)
		RETURN_ERROR(B_NOT_A_DIRECTORY);

	if (fatNode->de_FileSize == 0)
		return B_OK;

	mbnambuf longName;
		// filename after extraction and conversion from winentries
	mbnambuf_init(&longName);
	status_t status = B_OK;

	Vector<direntry> entries(50);
	status = read_dir_entries(bsdVolume, bsdNode, entries);

	uint16 carryover = 0;
	int chkSum = -1;

	for (int32 i = 0; i < entries.Count(); ++i) {
		direntry entry = entries.ElementAt(i);
		if (!is_winentry(&entry)) {
			_dump_direntry(&entry, i, printFunc);
			chkSum = -1;
			mbnambuf_init(&longName);
		} else {
			chkSum = _dump_winentry(reinterpret_cast<const winentry*>(&entry), i, chkSum, carryover,
				printFunc);
		}
	}
	count = entries.Count();

	RETURN_ERROR(status);
}


/*!	Print a list of all entries in a given directory.
	@param bsdNode The directory of interest.
	@pre \a bsdNode is at least read-locked.
	@post \a count is set to the number of entries printed, including entries marked as empty or
	deleted.
*/
status_t
dprintf_fat_dir(mount* bsdVolume, vnode* bsdNode, size_t& count)
{
	return _dump_fat_dir(bsdVolume, bsdNode, count, dprintf);
}
