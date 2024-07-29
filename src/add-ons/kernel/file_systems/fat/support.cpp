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


#include "support.h"

#ifdef FS_SHELL
#include "fssh_api_wrapper.h"
#else // !FS_SHELL
#include <dirent.h>
#include <malloc.h>

#include <NodeMonitor.h>
#include <SupportDefs.h>

#include <AutoDeleter.h>
#include <file_systems/mime_ext_table.h>
#include <real_time_clock.h>
#include <util/AutoLock.h>
#endif // !FS_SHELL

#include "debug.h"
#include "dosfs.h"
#include "vcache.h"


extern struct iconv_functions* msdosfs_iconv;

const char* LABEL_ILLEGAL = "\"*+,./:;<=>?[\\]|";

struct emptyDir gDirTemplate = {
	//	{	".          ",				/* the . entry */
	{
		{'.', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '},
		ATTR_DIRECTORY, /* file attribute */
		0, /* reserved */
		0, {0, 0}, {0, 0}, /* create time & date */
		{0, 0}, /* access date */
		{0, 0}, /* high bits of start cluster */
		{210, 4}, {210, 4}, /* modify time & date */
		{0, 0}, /* startcluster */
		{0, 0, 0, 0} /* filesize */
	},
	//	{	"..         ",				/* the .. entry */
	{
		{'.', '.', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '},
		ATTR_DIRECTORY, /* file attribute */
		0, /* reserved */
		0, {0, 0}, {0, 0}, /* create time & date */
		{0, 0}, /* access date */
		{0, 0}, /* high bits of start cluster */
		{210, 4}, {210, 4}, /* modify time & date */
		{0, 0}, /* startcluster */
		{0, 0, 0, 0} /* filesize */
	}};


ComponentName::ComponentName(u_int64_t flags, ucred* cred, enum nameiop nameiop, int lkflags,
	const char* nameptr)
{
	fData.cn_flags = flags;
	fData.cn_cred = cred;
	fData.cn_nameiop = nameiop;
	fData.cn_lkflags = lkflags;
	fData.cn_nameptr = strdup(nameptr);
	fData.cn_namelen = strlen(fData.cn_nameptr);
}


ComponentName::~ComponentName()
{
	free(fData.cn_nameptr);
}


struct componentname*
ComponentName::Data()
{
	return &fData;
}


bool
is_filename_legal(const char* name)
{
	if (name == NULL)
		return false;

	// illegal characters in a long file name
	const char illegal[] = "\\/:*?\"<>|";

	uint32 len = strlen(name);
	if (len <= 0)
		return false;

	// names with only an extension are not allowed
	if (strrchr(name, '.') == name)
		return false;

	// win2unixfn will trim trailing dots and spaces from the FAT longname
	// but it does expect there to be something there other than dots and spaces.
	bool hasContent = false;
	for (uint32 i = 0; i < len && hasContent == false; i++) {
		if (name[i] != ' ' && name[i] != '.')
			hasContent = true;
	}
	if (hasContent == false)
		return false;

	for (uint32 i = 0; i < len; i++) {
		if (name[i] & 0x80)
			continue; // belongs to an utf8 char
		if (strchr(illegal, name[i]))
			return false;
		if (static_cast<unsigned char>(name[i]) < 32)
			return false;
	}
	return true;
}


bool
is_shortname_legal(const u_char* name)
{
	// if the volume is ever mounted on Windows, short name should not be the same as that
	// of a Windows device
	const char* device_names[] = {"CON        ", "PRN        ", "AUX        ", "NUL        ",
		"COM0       ", "COM1       ", "COM2       ", "COM3       ", "COM4       ", "COM5       ",
		"COM6       ", "COM7       ", "COM8       ", "COM9       ", "LPT0       ", "LPT1       ",
		"LPT2       ", "LPT3       ", "LPT4       ", "LPT5       ", "LPT6       ", "LPT7       ",
		"LPT8       ", "LPT9       ", NULL};

	for (uint32 i = 0; device_names[i]; i++) {
		// only first 8 characters seem to matter
		if (memcmp(name, device_names[i], 8) == 0)
			return false;
	}

	return true;
}


/*! Convert a volume label from the format stored on disk into a normal string.
	@param name A character array of length 11 or a C string of size 12.
	@post Name is null-teriminated after the last non-space character, and converted to lower case.
*/
void
sanitize_label(char* name)
{
	int i;
	for (i = 10; i > 0; i--) {
		if (name[i] != ' ')
			break;
	}
	name[i + 1] = 0;

	for (; i >= 0; i--) {
		if (name[i] >= 'A' && name[i] <= 'Z')
			name[i] += 'a' - 'A';
	}
}


/*! Return the volume label.
	@param fd The device file, specified independently of volume because in
	dosfs_identify_partition, volume->pm_dev will not be initialized.
	@param label The pre-allocated (LABEL_CSTRING bytes) string into which the label is copied.
	@pre The following members of volume are initialized:  pm_rootdirsize, pm_BlkPerSec,
	pm_BytesPerSec, pm_bpcluster, pm_rootdirblk, pm_bnshift.
	@post If volume is an old DOS 3.3 era FAT volume, which will not have a label, then label is
	set to an empty string and the function returns B_OK. For other volumes, the root
	directory label is returned in label, if found; if not, the BPB label is returned.
*/
status_t
read_label(const msdosfsmount* volume, int fd, const uint8* buffer, char* label)
{
	*label = '\0';

	bool fat32 = FAT32(volume);
	uint8 check = fat32 == true ? 0x42 : 0x26;
	uint8 offset = fat32 == true ? 0x47 : 0x2b;
	if (buffer[check] == EXBOOTSIG && memcmp(buffer + offset, "           ", LABEL_LENGTH) != 0) {
		memcpy(label, buffer + offset, LABEL_LENGTH);
		sanitize_label(label);
	} else {
		return B_OK;
	}

	// read the root directory
	if (volume->pm_mountp != NULL)
		volume->pm_mountp->mnt_volentry = -1;
	int32 rootDirBytes;
	if (fat32 == true)
		rootDirBytes = volume->pm_bpcluster;
			// we will only search the first cluster of the root directory, for FAT32 volumes
	else
		rootDirBytes = (volume->pm_rootdirsize / volume->pm_BlkPerSec) * volume->pm_BytesPerSec;

	uint8* rootDirBuffer = static_cast<uint8*>(calloc(rootDirBytes, sizeof(char)));
	daddr_t rootDirBlock = volume->pm_rootdirblk;
	if (fat32 == true)
		rootDirBlock = cntobn(volume, rootDirBlock);
	off_t rootDirPos = de_bn2off(volume, rootDirBlock);

	int32 bytesRead = read_pos(fd, rootDirPos, rootDirBuffer, rootDirBytes);
	if (bytesRead != rootDirBytes) {
		free(rootDirBuffer);
		RETURN_ERROR(B_IO_ERROR);
	}

	// find volume label entry in the root directory (supercedes any label in the bpb)
	// continue silently if not found.
	uint32 direntrySize = sizeof(struct direntry);
	uint32 rootDirEntries = rootDirBytes / direntrySize;
	struct direntry* direntry = NULL;
	for (uint32 i = 0;
		i < rootDirEntries && (direntry == NULL || direntry->deName[0] != SLOT_EMPTY); ++i) {
		direntry = reinterpret_cast<struct direntry*>(rootDirBuffer + direntrySize * i);
		if (direntry->deName[0] != SLOT_DELETED && direntry->deAttributes != ATTR_WIN95
			&& (direntry->deAttributes & ATTR_VOLUME) != 0) {
			memcpy(label, direntry->deName, 11);
			sanitize_label(label);
			PRINT("found volume label in root directory:  %s at i = %" B_PRIu32 "\n", label, i);
			if (volume->pm_mountp != NULL)
				volume->pm_mountp->mnt_volentry = i;
			break;
		}
	}

	free(rootDirBuffer);

	return B_OK;
}


/*! Determine whether the bootsector is from a FAT volume and, if so, the type of FAT and, if
	FAT12 or FAT16, if it is formatted in the old DOS 3.3 format.
*/
status_t
check_bootsector(const uint8* bootsector, FatType& _type, bool& _dos33)
{
	// check the two FAT boot signature bytes
	if (bootsector[0x1fe] != BOOTSIG0 || bootsector[0x1ff] != BOOTSIG1)
		return B_BAD_VALUE;

	// check for indications of a non-FAT filesystem
	if (!memcmp((uint8*) bootsector + 3, "NTFS    ", 8)
		|| !memcmp((uint8*) bootsector + 3, "HPFS    ", 8)) {
		return B_BAD_VALUE;
	}

	// check the validity of a FAT BPB value to verify it is really FAT
	uint32 bytesPerSector = read16(bootsector, 0Xb);
	if (bytesPerSector != 512 && bytesPerSector != 1024 && bytesPerSector != 2048
		&& bytesPerSector != 4096) {
		return B_BAD_VALUE;
	}

	// It appears to be a valid FAT volume of some kind.
	// Determine the FAT type by counting the data clusters.
	uint32 sectorsPerCluster = bootsector[0xd];
	if (sectorsPerCluster == 0)
		return B_BAD_VALUE;
	uint32 rootDirEntries = read16(bootsector, 0x11);
	uint32 rootDirSectors = ((rootDirEntries * 32) + (bytesPerSector - 1)) / bytesPerSector;
	uint32 sectorsPerFat = read16(bootsector, 0x16);
	if (sectorsPerFat == 0)
		sectorsPerFat = read32(bootsector, 0x24);
	uint32 totalSectors = read16(bootsector, 0x13);
	if (totalSectors == 0)
		totalSectors = read32(bootsector, 0x20);
	uint32 reservedSectors = read16(bootsector, 0xe);
	uint32 fatCount = bootsector[0x10];
	uint32 dataSectors
		= totalSectors - (reservedSectors + (fatCount * sectorsPerFat) + rootDirSectors);
	uint32 clusterCount = dataSectors / sectorsPerCluster;
	if (clusterCount < 4085)
		_type = fat12;
	else if (clusterCount < 65525)
		_type = fat16;
	else
		_type = fat32;

	// determine whether we are dealing with an old DOS 3.3 format
	// volume by checking for the FAT extended boot signature
	if (_type != fat32 && bootsector[0x26] != EXBOOTSIG)
		_dos33 = true;
	else
		_dos33 = false;

	return B_OK;
}


/*! Populate volume with values from the BIOS parameter block.
	@pre Volume is already allocated and pm_fatmask has been set; if it is a temp object created
	for use in dosfs_identify_partition, that will be indicated by a NULL volume->pm_mount.
	@post Those members of volume that can be read directly from the BPB are initialized.
	pm_HugeSectors will be initialized, regardless of which	field the BPB stores the sector count
	in (in contrast, pm_Sectors will not always contain an accurate count).
*/
status_t
parse_bpb(msdosfsmount* volume, const bootsector* bootsector, bool dos33)
{
	status_t status = B_OK;
	const universal_byte_bpb* bpb
		= reinterpret_cast<const universal_byte_bpb*>(bootsector->bs33.bsBPB);

	// first fill in the universal fields from the bpb
	volume->pm_BytesPerSec = getushort(bpb->bpbBytesPerSec);
	volume->pm_bpb.bpbSecPerClust = bpb->bpbSecPerClust;
	volume->pm_ResSectors = getushort(bpb->bpbResSectors);
	volume->pm_FATs = bpb->bpbFATs;
	volume->pm_RootDirEnts = getushort(bpb->bpbRootDirEnts);
	volume->pm_Sectors = getushort(bpb->bpbSectors);
	volume->pm_Media = bpb->bpbMedia;
	volume->pm_FATsecs = volume->pm_bpb.bpbFATsecs = getushort(bpb->bpbFATsecs);
	volume->pm_SecPerTrack = getushort(bpb->bpbSecPerTrack);
	volume->pm_Heads = getushort(bpb->bpbHeads);

	// check the validity of some universal fields
	if (volume->pm_BytesPerSec != 0x200 && volume->pm_BytesPerSec != 0x400
		&& volume->pm_BytesPerSec != 0x800 && volume->pm_BytesPerSec != 0x1000) {
		INFORM("invalid bytes per sector (%d)\n", volume->pm_BytesPerSec);
		status = B_BAD_VALUE;
	} else if (volume->pm_BytesPerSec < DEV_BSIZE) {
		INFORM("invalid bytes per sector (%d)\n", volume->pm_BytesPerSec);
		status = B_BAD_VALUE;
	} else if (volume->pm_bpb.bpbSecPerClust != 1 && volume->pm_bpb.bpbSecPerClust != 2
		&& volume->pm_bpb.bpbSecPerClust != 4 && volume->pm_bpb.bpbSecPerClust != 8
		&& volume->pm_bpb.bpbSecPerClust != 16 && volume->pm_bpb.bpbSecPerClust != 32
		&& volume->pm_bpb.bpbSecPerClust != 64 && volume->pm_bpb.bpbSecPerClust != 128) {
		INFORM("invalid sectors per cluster (%" B_PRIu8 ")\n", volume->pm_bpb.bpbSecPerClust);
		status = B_BAD_VALUE;
	} else if (volume->pm_BytesPerSec * volume->pm_bpb.bpbSecPerClust > 0x10000) {
		INFORM("invalid bytes per cluster (%" B_PRIu16 ")\n",
			volume->pm_BytesPerSec * volume->pm_bpb.bpbSecPerClust);
		status = B_BAD_VALUE;
	} else if (volume->pm_FATs == 0 || volume->pm_FATs > 8) {
		INFORM("unreasonable fat count (%" B_PRIu8 ")\n", volume->pm_FATs);
		status = B_BAD_VALUE;
	} else if (volume->pm_Media != 0xf0 && volume->pm_Media < 0xf8) {
		INFORM("invalid media descriptor byte\n");
		status = B_BAD_VALUE;
	}
	if (status != B_OK)
		return status;

	// now become more discerning citizens
	if (dos33 == true && (FAT12(volume) != 0 || FAT16(volume) != 0)) {
		const struct byte_bpb33* b33 = reinterpret_cast<const byte_bpb33*>(bootsector->bs33.bsBPB);
		if (volume->pm_Sectors == 0) {
			INFORM("sector count missing\n");
			status = B_BAD_VALUE;
		} else {
			volume->pm_HugeSectors = volume->pm_Sectors;
				// The driver relies on pm_HugeSectors containing the sector count,
				// regardless of which BPB field the sector count is stored in.
			volume->pm_HiddenSects = getushort(b33->bpbHiddenSecs);
			volume->pm_flags |= MSDOSFS_FATMIRROR;
		}
	} else if (FAT12(volume) != 0 || FAT16(volume) != 0) {
		const struct byte_bpb50* b50 = reinterpret_cast<const byte_bpb50*>(bootsector->bs50.bsBPB);
		if (volume->pm_Sectors == 0)
			volume->pm_HugeSectors = getulong(b50->bpbHugeSectors);
		else
			volume->pm_HugeSectors = volume->pm_Sectors;
		volume->pm_HiddenSects = getulong(b50->bpbHiddenSecs);
		volume->pm_flags |= MSDOSFS_FATMIRROR;
		if (FAT16(volume) && volume->pm_mountp != NULL)
			fix_zip(b50, volume);
	} else if (FAT32(volume) != 0) {
		const struct byte_bpb710* b710
			= reinterpret_cast<const byte_bpb710*>(bootsector->bs710.bsBPB);
		volume->pm_HiddenSects = getulong(b710->bpbHiddenSecs);
		volume->pm_HugeSectors = getulong(b710->bpbHugeSectors);
		volume->pm_FATsecs = getulong(b710->bpbBigFATsecs);
			// overwrite the contents of the 16-bit FATsecs BPB field, which would be 0 for FAT32
		if ((getushort(b710->bpbExtFlags) & FATMIRROR) != 0)
			volume->pm_curfat = getushort(b710->bpbExtFlags) & FATNUM;
		else
			volume->pm_flags |= MSDOSFS_FATMIRROR;
		volume->pm_rootdirblk = getulong(b710->bpbRootClust);
		volume->pm_fsinfo = getushort(b710->bpbFSInfo);
	} else {
		status = B_UNSUPPORTED;
	}

	if (status != B_OK)
		return status;

	// check the validity of some type-specific fields
	if (FAT12(volume) || FAT16(volume)) {
		if ((volume->pm_RootDirEnts * 32) % volume->pm_BytesPerSec != 0) {
			INFORM("invalid number of root dir entries\n");
			status = B_BAD_VALUE;
		}
	} else if (FAT32(volume)) {
		const struct byte_bpb710* b710
			= reinterpret_cast<const byte_bpb710*>(bootsector->bs710.bsBPB);
		if (getushort(b710->bpbSectors) != 0 || getushort(b710->bpbFSVers) != 0
			|| getushort(b710->bpbRootDirEnts) != 0 || getushort(b710->bpbFATsecs) != 0) {
			INFORM("bad FAT32 filesystem\n");
			status = B_BAD_VALUE;
		}
	}
	if ((off_t) volume->pm_HugeSectors * volume->pm_BytesPerSec < volume->pm_HugeSectors) {
		INFORM("sectors overflow\n");
		status = B_BAD_VALUE;
	}
	if (volume->pm_mountp != NULL) {
		if (static_cast<off_t>(volume->pm_HugeSectors) * volume->pm_BytesPerSec
			> volume->pm_dev->si_mediasize) {
			INFORM("sectors past end of vol:  %u sectors on a %" B_PRIdOFF "-byte volume\n",
				volume->pm_HugeSectors, volume->pm_dev->si_mediasize);
			status = B_BAD_VALUE;
		}
	}

	return status;
}


/*! Zip disks that were formatted at iomega have an incorrect number of sectors.
	They say that they have 196576 sectors but they really only have 196192.
	This check is a work-around for their brain-deadness.
	@pre volume->pm_HugeSectors and volume->pm_dev have been initialized.
*/
void
fix_zip(const byte_bpb50* bpb, msdosfsmount* volume)
{
	device_geometry* geo = volume->pm_dev->si_geometry;

	if (geo != NULL) {
		// the BPB values that are characteristic of these disks
		unsigned char bogusZipData[] = {0x00, 0x02, 0x04, 0x01, 0x00, 0x02, 0x00, 0x02, 0x00, 0x00,
			0xf8, 0xc0, 0x00, 0x20, 0x00, 0x40, 0x00, 0x20, 0x00, 0x00};

		if (memcmp(bpb, bogusZipData, sizeof(bogusZipData)) == 0 && volume->pm_HugeSectors == 196576
			&& (static_cast<off_t>(geo->sectors_per_track) * static_cast<off_t>(geo->cylinder_count)
				   * static_cast<off_t>(geo->head_count))
				== 196192) {
			volume->pm_HugeSectors = 196192;
		}
	}

	return;
}


/*! Collect data from the fsinfo sector, if applicable.
	@pre devNode and the block cache are initialized. The volume is still in the process
	of being mounted, so we have exclusive access to the sector (there is no need for a read lock).
*/
status_t
read_fsinfo(msdosfsmount* volume, const vnode* devNode)
{
	status_t status = B_OK;

	if (FAT32(volume) != 0) {
		const uint8* buffer;
		const struct fsinfo* fsInfo;

		status = block_cache_get_etc(volume->pm_mountp->mnt_cache, volume->pm_fsinfo, 0, 1,
			reinterpret_cast<const void**>(&buffer));
		if (status != B_OK)
			RETURN_ERROR(status);

		fsInfo = reinterpret_cast<const fsinfo*>(buffer);
		if (memcmp(fsInfo->fsisig1, "RRaA", 4) == 0 && memcmp(fsInfo->fsisig2, "rrAa", 4) == 0
			&& memcmp(fsInfo->fsisig3, "\0\0\125\252", 4) == 0) {
			volume->pm_nxtfree = getulong(fsInfo->fsinxtfree);
			if (volume->pm_nxtfree > volume->pm_maxcluster)
				volume->pm_nxtfree = CLUST_FIRST;
			// fillinusemap will populate pm_freeclustercount
		} else {
			INFORM("fsinfo block has invalid magic number\n");
			status = B_BAD_VALUE;
			volume->pm_fsinfo = 0;
		}

		block_cache_put(volume->pm_mountp->mnt_cache, volume->pm_fsinfo);
	}

	/*
	 * Finish initializing pmp->pm_nxtfree (just in case the first few
	 * sectors aren't properly reserved in the FAT). This completes
	 * the fixup for fp->fsinxtfree, and fixes up the zero-initialized
	 * value if there is no fsinfo. We will use pmp->pm_nxtfree
	 * internally even if there is no fsinfo.
	 */
	if (volume->pm_nxtfree < CLUST_FIRST)
		volume->pm_nxtfree = CLUST_FIRST;

	return status;
}


/*! Update the fsinfo sector, if applicable.
	@pre the block cache is initialized.
*/
status_t
write_fsinfo(msdosfsmount* volume)
{
	if (volume->pm_fsinfo == 0 || FAT32(volume) == false || (volume->pm_flags & MSDOSFS_FSIMOD) == 0
		|| (volume->pm_flags & MSDOSFSMNT_RONLY) != 0) {
		return B_OK;
	}

	void* buffer = block_cache_get_writable(volume->pm_mountp->mnt_cache, volume->pm_fsinfo, -1);
	if (buffer == NULL)
		RETURN_ERROR(B_ERROR);

	struct fsinfo* fsInfo = reinterpret_cast<struct fsinfo*>(buffer);
	if (memcmp(fsInfo->fsisig1, "RRaA", 4) != 0 || memcmp(fsInfo->fsisig2, "rrAa", 4) != 0
		|| memcmp(fsInfo->fsisig3, "\0\0\125\252", 4) != 0) {
		block_cache_set_dirty(volume->pm_mountp->mnt_cache, volume->pm_fsinfo, false, -1);
		block_cache_put(volume->pm_mountp->mnt_cache, volume->pm_fsinfo);
		RETURN_ERROR(B_ERROR);
	}

	putulong(fsInfo->fsinfree, volume->pm_freeclustercount);
	putulong(fsInfo->fsinxtfree, volume->pm_nxtfree);
	volume->pm_flags &= ~MSDOSFS_FSIMOD;

	block_cache_put(volume->pm_mountp->mnt_cache, volume->pm_fsinfo);

	return B_OK;
}


/*! Perform sanity checks on the FAT.

*/
status_t
check_fat(const msdosfsmount* volume)
{
	uint8 fatBuffer[512];
	uint8 mirrorBuffer[512];

	// for each block
	for (uint32 i = 0; i < volume->pm_FATsecs; ++i) {
		// read a block from the first/active fat
		uint32 resBlocks = volume->pm_ResSectors * volume->pm_BlkPerSec;
		off_t position = 512 * (resBlocks + volume->pm_curfat * volume->pm_FATsecs + i);
		ssize_t bytes_read
			= read_pos(volume->pm_dev->si_fd, position, reinterpret_cast<void*>(fatBuffer), 0x200);
		if (bytes_read != 0x200)
			RETURN_ERROR(B_IO_ERROR);

		if (i == 0 && fatBuffer[0] != volume->pm_Media) {
			INFORM("media descriptor mismatch (%x != %x)\n", fatBuffer[0], volume->pm_Media);
			return B_BAD_VALUE;
		}

		// for each mirror
		for (uint32 j = 1; j < volume->pm_FATs; ++j) {
			position = 512 * (resBlocks + volume->pm_FATsecs * j + i);
			bytes_read = read_pos(volume->pm_dev->si_fd, position,
				reinterpret_cast<void*>(mirrorBuffer), 0x200);
			if (bytes_read != 0x200)
				RETURN_ERROR(B_IO_ERROR);

			if (i == 0 && mirrorBuffer[0] != volume->pm_Media) {
				INFORM("dosfs error: media descriptor mismatch in fat # "
					   "%" B_PRIu32 " (%" B_PRIu8 " != %" B_PRIu8 ")\n",
					j, mirrorBuffer[0], volume->pm_Media);
				return B_BAD_VALUE;
			}

			// checking for exact matches of fats is too
			// restrictive; allow these to go through in
			// case the fat is corrupted for some reason
			if (memcmp(fatBuffer, mirrorBuffer, 0x200)) {
				INFORM("FAT %" B_PRIu32 " doesn't match active FAT (%u) on %s.\n"
					   "Install dosfstools and use fsck.fat to inspect %s.\n",
					j, volume->pm_curfat, volume->pm_dev->si_device, volume->pm_dev->si_device);
			}
		}
	}

	return B_OK;
}


/*! Traverse n fat entries.
	E.g. for n = 1, return the cluster that is next in the chain after 'cluster'
	If 'cluster' is the last in the chain, returns the last-cluster value.
*/
uint32
get_nth_fat_entry(msdosfsmount* fatVolume, uint32 cluster, uint32 n)
{
	int status = 0;

	ReadLocker locker(fatVolume->pm_fatlock.haikuRW);

	u_long resultCluster = static_cast<u_long>(cluster);

	while (n--) {
		status = fatentry(FAT_GET, fatVolume, resultCluster, &resultCluster, 0);
		if (status != 0) {
			REPORT_ERROR(B_FROM_POSIX_ERROR(status));
			break;
		}
		ASSERT(IS_DATA_CLUSTER(resultCluster));
	}

	return static_cast<uint32>(resultCluster);
}


status_t
init_csi(msdosfsmount* fatVolume, uint32 cluster, uint32 sector, struct csi* csi)
{
	int ret;
	if ((ret = validate_cs(fatVolume, cluster, sector)) != B_OK)
		return ret;

	csi->fatVolume = fatVolume;
	csi->cluster = cluster;
	csi->sector = sector;

	return B_OK;
}


status_t
validate_cs(msdosfsmount* fatVolume, uint32 cluster, uint32 sector)
{
	if (cluster == MSDOSFSROOT) {
		if (sector >= fatVolume->pm_rootdirsize / fatVolume->pm_BlkPerSec)
			return B_ERROR;
		return B_OK;
	}

	if (sector >= SECTORS_PER_CLUSTER(fatVolume)) {
		INFORM("validate_cs:  invalid sector (%" B_PRIu32 ")\n", sector);
		return B_ERROR;
	}

	if (!IS_DATA_CLUSTER(cluster)) {
		INFORM("validate_cs:  cluster %" B_PRIu32 " compared to max %lu\n", cluster,
			fatVolume->pm_maxcluster);
		return B_ERROR;
	}

	return B_OK;
}


/*! Return the filesystem-relative sector number that corresponds to the argument.

*/
off_t
fs_sector(struct csi* csi)
{
	ASSERT(validate_cs(csi->fatVolume, csi->cluster, csi->sector) == 0);

	if (csi->cluster == MSDOSFSROOT) {
		return static_cast<off_t>(csi->fatVolume->pm_rootdirblk) / csi->fatVolume->pm_BlkPerSec
			+ csi->sector;
	}

	off_t clusterStart
		= cntobn(csi->fatVolume, static_cast<off_t>(csi->cluster)) / csi->fatVolume->pm_BlkPerSec;

	return clusterStart + csi->sector;
}


/*! Advance csi by the indicated number of sectors.
	If it's not possible to advance this many sectors the function returns B_ERROR.
*/
status_t
iter_csi(struct csi* csi, int sectors)
{
	if (csi->sector == 0xffff) // check if already at end of chain
		return B_ERROR;

	if (sectors < 0)
		return B_BAD_VALUE;

	if (sectors == 0)
		return B_OK;

	if (csi->cluster == MSDOSFSROOT) {
		csi->sector += sectors;
		if (csi->sector < csi->fatVolume->pm_rootdirsize / csi->fatVolume->pm_BlkPerSec)
			return B_OK;
	} else {
		u_long secPerClust = SECTORS_PER_CLUSTER(csi->fatVolume);
		csi->sector += sectors;
		if (csi->sector < secPerClust)
			return B_OK;
		csi->cluster = get_nth_fat_entry(csi->fatVolume, csi->cluster, csi->sector / secPerClust);

		if (MSDOSFSEOF(csi->fatVolume, csi->cluster)) {
			csi->sector = 0xffff;
			return B_ERROR;
		}

		if (vIS_DATA_CLUSTER(csi->fatVolume, csi->cluster)) {
			csi->sector %= SECTORS_PER_CLUSTER(csi->fatVolume);
			return B_OK;
		}
	}

	csi->sector = 0xffff;

	return B_ERROR;
}


NodePutter::NodePutter()
	:
	fNode(NULL)
{
}


NodePutter::NodePutter(vnode* node)
	:
	fNode(node)
{
}


NodePutter::~NodePutter()
{
	Put();
}


void
NodePutter::SetTo(vnode* node)
{
	fNode = node;
}


void
NodePutter::Put()
{
	if (fNode != NULL) {
		fs_volume* volume = fNode->v_mount->mnt_fsvolume;
		denode* fatNode = reinterpret_cast<denode*>(fNode->v_data);
		ino_t ino = static_cast<ino_t>(fatNode->de_inode);
		status_t status = put_vnode(volume, ino);
		if (status != B_OK)
			REPORT_ERROR(status);
		fNode = NULL;
	}
}


/*! Given a tentative inode number based on direntry location, set it the final inode number.
	The final number will be different if the directory entry slot of a deleted or renamed file
	is re-used. Also add the node to the vcache if it isn't already added. Unlike the original
	Haiku FAT driver, each node is added to the vcache (not just those with artificial ID #s).
*/
status_t
assign_inode(mount* volume, ino_t* inode)
{
	ino_t location = *inode;
	status_t result = B_OK;

	if (*inode == root_inode(reinterpret_cast<msdosfsmount*>(volume->mnt_data)))
		return B_OK;

	// populate finalInode with the ID of the node with this direntry location, if
	// such a mapping exists in the vcache
	ino_t finalInode = 0;
	result = vcache_loc_to_vnid(volume, location, &finalInode);
	if (result == ENOENT) {
		// Search again, this time using the location as an ID.
		// If an entry is found, then we know that this location is being used
		// as an ID by some (other) node.
		if (find_vnid_in_vcache(volume, location) == B_OK) {
			// The location cannot be used as an ID because it is already in use.
			// Assign an artificial ID.
			finalInode = generate_unique_vnid(volume);
			if ((result = add_to_vcache(volume, finalInode, location)) < 0)
				RETURN_ERROR(result);
		} else {
			// otherwise we are free to use it
			finalInode = location;
			if ((result = add_to_vcache(volume, finalInode, location)) < 0)
				RETURN_ERROR(result);
		}
	} else if (result != B_OK) {
		RETURN_ERROR(result);
	}

	PRINT("assign_inode in:  %" B_PRIdINO ", out:  %" B_PRIdINO "\n", *inode, finalInode);
	*inode = finalInode;

	return B_OK;
}


status_t
assign_inode_and_get(mount* bsdVolume, daddr_t cluster, u_long offset, vnode** bsdNode)
{
	msdosfsmount* fatVolume = reinterpret_cast<msdosfsmount*>(bsdVolume->mnt_data);
	ino_t ino = DETOI(fatVolume, cluster, offset);
	status_t status = assign_inode(bsdVolume, &ino);
	if (status != B_OK)
		RETURN_ERROR(B_ENTRY_NOT_FOUND);
	fs_volume* fsVolume = bsdVolume->mnt_fsvolume;
	status = get_vnode(fsVolume, ino, reinterpret_cast<void**>(bsdNode));
	if (status != B_OK)
		RETURN_ERROR(B_ENTRY_NOT_FOUND);
	return B_OK;
}


/*! Convert inode into direntry location.
	@param vnid A final inode number (possibly location-based, possibly artificial).
	@post dirclust and diroffset specify the location of the direntry associated with vnid. If the
	location is in the (FAT12/16) root directory, diroffset is relative to the start of the root
	directory; otherwise it is cluster-relative.
*/
status_t
get_location(mount* bsdVolume, ino_t vnid, u_long* dirclust, u_long* diroffset)
{
	msdosfsmount* fatVolume = reinterpret_cast<msdosfsmount*>(bsdVolume->mnt_data);
	ino_t location = -1;

	vcache_vnid_to_loc(bsdVolume, vnid, &location);

	if (dirclust != NULL && diroffset != NULL) {
		// do the reverse of DETOI
		if (static_cast<unsigned long>(location) < fatVolume->pm_RootDirEnts) {
			// this is a regular file in a fixed root directory
			*dirclust = MSDOSFSROOT;
			*diroffset = location << 5;
		} else {
			location -= fatVolume->pm_RootDirEnts;
			location <<= 5;
			*dirclust = (location / fatVolume->pm_bpcluster) + 2;
			*diroffset = location % fatVolume->pm_bpcluster;
		}
	}

	return B_OK;
}


bool
node_exists(mount* bsdVolume, uint64_t inode)
{
	bool constructed = false;
	vcache_get_constructed(bsdVolume, inode, &constructed);

	return constructed;
}


/*! Analagous to FreeBSD's vfs_timestamp, but returns the local time instead of GMT.

*/
void
timestamp_local(timespec* tsp)
{
	bigtime_t usecs = real_time_clock_usecs();
	tsp->tv_sec = (usecs / 1000000LL);
	tsp->tv_nsec = (usecs - tsp->tv_sec * 1000000LL) * 1000LL;

	return;
}


void
local_to_GMT(const timespec* tspLocal, timespec* tspGMT)
{
	*tspGMT = *tspLocal;

	int32 offset = 0;

#ifdef _KERNEL_MODE
	offset = static_cast<int32>(get_timezone_offset());
#elif defined USER
	time_t localTime;
	time(&localTime);

	struct tm localTm;
	localtime_r(&localTime, &localTm);
	offset = localTm.tm_gmtoff;
#endif

	tspGMT->tv_sec -= offset;

	return;
}


/*! Allocate and insert a struct buf into a buffer list.
	@param deviceNode The node that represents the mounted device, whose v_bufobj member is to be
	updated.
	@param size The required size of buf::b_data.
	@pre The msdosfsmount has been initialized and v_bufobj.bo_lock is write locked.
	@post A new buf is inserted at the head of the bufobj SLIST that corresponds to 'size.'  C-style
	allocation so the buf can be freed in C code.
*/
status_t
slist_insert_buf(vnode* deviceNode, size_t size)
{
	buf_list* list = NULL;
	uint32* count = NULL;
	msdosfsmount* fatVolume
		= reinterpret_cast<msdosfsmount*>(deviceNode->v_rdev->si_mountpt->mnt_data);

	if (size == 0) {
		list = &deviceNode->v_bufobj.bo_emptybufs;
		count = &deviceNode->v_bufobj.bo_empties;
	} else if (size == fatVolume->pm_fatblocksize) {
		list = &deviceNode->v_bufobj.bo_fatbufs;
		count = &deviceNode->v_bufobj.bo_fatblocks;
	} else if (size == fatVolume->pm_bpcluster) {
		list = &deviceNode->v_bufobj.bo_clusterbufs;
		count = &deviceNode->v_bufobj.bo_clusters;
	} else {
		return B_UNSUPPORTED;
	}

	buf* newBuf = reinterpret_cast<buf*>(calloc(1, sizeof(buf)));
	if (newBuf == NULL)
		return B_NO_MEMORY;
	if (size != 0) {
		newBuf->b_data = reinterpret_cast<caddr_t>(calloc(size, sizeof(char)));
		if (newBuf->b_data == NULL) {
			free(newBuf);
			return B_NO_MEMORY;
		}
	}
	newBuf->b_bufsize = size;
	// The other members of newBuf will be initialized by getblkx

	SLIST_INSERT_HEAD(list, newBuf, link);
	(*count)++;

	return B_OK;
}


status_t
fill_gap_with_zeros(vnode* bsdNode, off_t pos, off_t newSize)
{
	while (pos < newSize) {
		size_t size;
		if (newSize > pos + 1024 * 1024 * 1024)
			size = 1024 * 1024 * 1024;
		else
			size = newSize - pos;

		status_t status = file_cache_write(bsdNode->v_cache, NULL, pos, NULL, &size);
		if (status < B_OK)
			return status;

		pos += size;
	}

	return B_OK;
}


/*! Sync the block cache blocks that hold the data of a directory file.

*/
status_t
sync_clusters(vnode* bsdNode)
{
	mount* bsdVolume = bsdNode->v_mount;
	denode* fatNode = reinterpret_cast<denode*>(bsdNode->v_data);
	msdosfsmount* fatVolume = fatNode->de_pmp;
	status_t status = B_OK;

	ASSERT(bsdNode->v_type == VDIR);

	u_long cluster = fatNode->de_dirclust;

	if (cluster == MSDOSFSROOT) {
		status = block_cache_sync_etc(bsdVolume->mnt_cache, fatVolume->pm_rootdirblk,
			fatVolume->pm_rootdirsize);
	} else {
		status_t fatStatus = B_OK;
		while ((IS_DATA_CLUSTER(cluster)) && status == B_OK && fatStatus == B_OK) {
			status = block_cache_sync_etc(bsdVolume->mnt_cache, de_cn2bn(fatVolume, cluster),
				BLOCKS_PER_CLUSTER(fatVolume));
			fatStatus = B_FROM_POSIX_ERROR(fatentry(FAT_GET, fatVolume, cluster, &cluster, 0));
		}
		if (fatStatus != B_OK)
			REPORT_ERROR(fatStatus);
	}

	RETURN_ERROR(status);
}


/*! Discard the block cache blocks that hold a directory's data. Has no effect on the root
	directory in FAT12/16; those blocks will be discarded when the volume is unmounted.
*/
status_t
discard_clusters(vnode* bsdNode, off_t newLength)
{
	mount* bsdVolume = bsdNode->v_mount;
	denode* fatNode = reinterpret_cast<denode*>(bsdNode->v_data);
	msdosfsmount* fatVolume = fatNode->de_pmp;
	status_t status = B_OK;

	ASSERT(bsdNode->v_type == VDIR);

	// if we arrived here from detrunc, de_StartCluster has already been reset.
	u_long cluster = fatNode->de_dirclust;

	// Typically we are discarding all clusters associated with a directory. However, in
	// the case of an error, the driver might shrink a directory to undo an attempted expansion,
	// as in createde.
	for (uint32 skip = howmany(newLength, fatVolume->pm_bpcluster); skip > 0 && status == B_OK;
		skip--) {
		status = B_FROM_POSIX_ERROR(fatentry(FAT_GET, fatVolume, cluster, &cluster, 0));
	}

	while ((IS_DATA_CLUSTER(cluster)) && status == B_OK) {
		block_cache_discard(bsdVolume->mnt_cache, de_cn2bn(fatVolume, cluster),
			BLOCKS_PER_CLUSTER(fatVolume));
		status = B_FROM_POSIX_ERROR(fatentry(FAT_GET, fatVolume, cluster, &cluster, 0));
	}

	RETURN_ERROR(status);
}


/*! For use in the FAT userlandfs module. userlandfs does not provide check_access_permissions().
	Limitation:  ignores permissions granted by the user's group
*/
status_t
check_access_permissions_internal(int accessMode, mode_t mode, gid_t nodeGroupID, uid_t nodeUserID)
{
	// get node permissions
	int userPermissions = (mode & S_IRWXU) >> 6;
	int groupPermissions = (mode & S_IRWXG) >> 3;
	int otherPermissions = mode & S_IRWXO;

	// get the node permissions for this uid/gid
	int permissions = 0;
	uid_t uid = geteuid();

	if (uid == 0) {
		// user is root
		// root has always read/write permission, but at least one of the
		// X bits must be set for execute permission
		permissions = userPermissions | groupPermissions | otherPermissions | S_IROTH | S_IWOTH;
		if (S_ISDIR(mode))
			permissions |= S_IXOTH;
	} else if (uid == nodeUserID) {
		// user is node owner
		permissions = userPermissions;
	} else {
		// user is one of the others
		permissions = otherPermissions;
	}

	// userlandfs does not provide is_user_in_group(), so we can't check group permissions

	return (accessMode & ~permissions) == 0 ? B_OK : B_PERMISSION_DENIED;
}


/*! Populates file mode bits only (not file type bits).

*/
void
mode_bits(const vnode* bsdNode, mode_t* mode)
{
	denode* fatNode = reinterpret_cast<denode*>(bsdNode->v_data);
	msdosfsmount* fatVolume = fatNode->de_pmp;

	*mode = S_IRUSR | S_IRGRP | S_IROTH;

	if ((fatNode->de_Attributes & ATTR_READONLY) == 0)
		*mode |= S_IWUSR;

	// In FAT, there is no place to store an executable flag on disk. FreeBSD makes all FAT files
	// executable, but Tracker will complain if, for example, a text file is executable.
	// To avoid that, we go by the MIME type.
	if (bsdNode->v_type == VDIR
		|| (bsdNode->v_type == VREG && bsdNode->v_mime != NULL
			&& strcmp(bsdNode->v_mime, "application/octet-stream") == 0)) {
		*mode |= S_IXUSR | S_IXGRP | S_IXOTH;
	}

	*mode &= (bsdNode->v_type == VDIR) ? fatVolume->pm_dirmask : fatVolume->pm_mask;

	return;
}


/*! Set the mime type of a node; has no effect in fat_shell.
	@param update True if this is an update to a pre-existing mime setting.
*/
status_t
set_mime_type(vnode* bsdNode, bool update)
{
#ifndef FS_SHELL
	mount* bsdVolume = reinterpret_cast<mount*>(bsdNode->v_mount);
	denode* fatNode = reinterpret_cast<denode*>(bsdNode->v_data);
	msdosfsmount* fatVolume = reinterpret_cast<msdosfsmount*>(fatNode->de_pmp);

	if (bsdNode->v_type == VREG) {
		char unixShortname[SHORTNAME_CSTRING + 1];
			// +1 for the period added by dos2unixfn
		dos2unixfn(fatNode->de_Name, reinterpret_cast<u_char*>(unixShortname), 0, fatVolume);

		set_mime(&bsdNode->v_mime, unixShortname);

		notify_attribute_changed(bsdVolume->mnt_fsvolume->id, bsdNode->v_parent, fatNode->de_inode,
			"BEOS:TYPE", update ? B_ATTR_CHANGED : B_ATTR_CREATED);
	}
#endif // FS_SHELL

	return B_OK;
}


/*! Set a user-specified code page for translating FAT short filenames in UserlandFS.
	The FAT filesystem assigns both a short name and a long name to each file/directory.
	The short names are encoded in an 8-bit or DBCS OEM code page, aka DOS code page.
	Short names must be unique within a directory.
	If the user assigns a name containing a character not in the OEM code page, then
	the short name will substitute an underscore character.
	Users only see the long name, but collisions between short entry names are more likely if
	the OEM code page is not suitable for the user's language.
	FAT FS will attempt to resolve collisions by adding a generation number to the new
	short filename (also invisible to the user).
	Many code pages are supported by libiconv, which we use in the the userlandfs module.
	libiconv is not available in the FS shell or the kernel; in those cases the driver defaults to
	an internal copy of code page 850.
*/
status_t
iconv_init(msdosfsmount* fatVolume, const char* oemPreference)
{
	fatVolume->pm_u2w = NULL;
	fatVolume->pm_w2u = NULL;
	fatVolume->pm_u2d = NULL;
	fatVolume->pm_d2u = NULL;

#ifndef USER
	return B_OK;
#else
	if ((fatVolume->pm_flags & MSDOSFSMNT_KICONV) == 0 || strcmp(oemPreference, "") == 0)
		return B_OK;

	msdosfs_iconv = new(std::nothrow) iconv_functions;
		// fills in for FreeBSD's VFS_DECLARE_ICONV macro
	if (msdosfs_iconv == NULL)
		RETURN_ERROR(B_NO_MEMORY);
	ObjectDeleter<iconv_functions> deleter(msdosfs_iconv);

	msdosfs_iconv->open = fat_iconv_open;
	msdosfs_iconv->close = fat_iconv_close;
	msdosfs_iconv->conv = iconv_conv;
	msdosfs_iconv->convchr = iconv_convchr;
	msdosfs_iconv->convchr_case = iconv_convchr_case;

	PRINT("setting code page to %s\n", oemPreference);

	const char* haiku = "UTF-8";
	const char* windows = "UCS-2";
	const char* dos = oemPreference;

	status_t status = B_OK;
	status = B_FROM_POSIX_ERROR(msdosfs_iconv->open(windows, haiku, &fatVolume->pm_u2w));
	if (status != B_OK)
		RETURN_ERROR(errno);

	status = B_FROM_POSIX_ERROR(msdosfs_iconv->open(haiku, windows, &fatVolume->pm_w2u));
	if (status != B_OK)
		RETURN_ERROR(errno);

	status = B_FROM_POSIX_ERROR(msdosfs_iconv->open(dos, haiku, &fatVolume->pm_u2d));
	if (status != B_OK)
		RETURN_ERROR(errno);

	status = B_FROM_POSIX_ERROR(msdosfs_iconv->open(haiku, dos, &fatVolume->pm_d2u));
	if (status != B_OK)
		RETURN_ERROR(errno);

	deleter.Detach();

	return B_OK;
#endif // USER
}
