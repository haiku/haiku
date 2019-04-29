/*
 * Copyright 2011, Jérôme Duval, korli@users.berlios.de.
 * Copyright 2008-2014, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 */


#include "Inode.h"

#include <real_time_clock.h>
#include <string.h>
#include <stdlib.h>

#include "CachedBlock.h"
#include "DataStream.h"
#include "Utility.h"


#undef ASSERT
//#define TRACE_EXFAT
#ifdef TRACE_EXFAT
#	define TRACE(x...) dprintf("\33[34mexfat:\33[0m " x)
#	define ASSERT(x) { if (!(x)) kernel_debugger("exfat: assert failed: " #x "\n"); }
#else
#	define TRACE(x...) ;
#	define ASSERT(x) ;
#endif
#define ERROR(x...) dprintf("\33[34mexfat:\33[0m " x)


Inode::Inode(Volume* volume, cluster_t cluster, uint32 offset)
	:
	fVolume(volume),
	fID(volume->GetIno(cluster, offset, 0)),
	fCluster(cluster),
	fOffset(offset),
	fCache(NULL),
	fMap(NULL)
{
	TRACE("Inode::Inode(%" B_PRIu32 ", %" B_PRIu32 ") inode %" B_PRIdINO "\n",
		Cluster(), Offset(), ID());
	_Init();

	if (ID() == 1) {
		fFileEntry.file.SetAttribs(EXFAT_ENTRY_ATTRIB_SUBDIR);
		fFileEntry.file_info.SetStartCluster(Cluster());
		fFileInfoEntry.file_info.SetFlag(0);
	} else {
		fInitStatus = UpdateNodeFromDisk();
		if (fInitStatus == B_OK && !IsDirectory() && !IsSymLink()) {
			fCache = file_cache_create(fVolume->ID(), ID(), Size());
			fMap = file_map_create(fVolume->ID(), ID(), Size());
		}
	}
	TRACE("Inode::Inode(%" B_PRIdINO ") end\n", ID());
}


Inode::Inode(Volume* volume, ino_t ino)
	:
	fVolume(volume),
	fID(ino),
	fCluster(0),
	fOffset(0),
	fCache(NULL),
	fMap(NULL),
	fInitStatus(B_NO_INIT)
{
	struct node_key *key = volume->GetNode(ino, fParent);
	if (key != NULL) {
		fCluster = key->cluster;
		fOffset = key->offset;
		fInitStatus = B_OK;
	}
	TRACE("Inode::Inode(%" B_PRIdINO ") cluster %" B_PRIu32 "\n", ID(),
		Cluster());
	_Init();

	if (fInitStatus == B_OK && ID() != 1) {
		fInitStatus = UpdateNodeFromDisk();
		if (!IsDirectory() && !IsSymLink()) {
			fCache = file_cache_create(fVolume->ID(), ID(), Size());
			fMap = file_map_create(fVolume->ID(), ID(), Size());
		}
	} else if (fInitStatus == B_OK && ID() == 1) {
		fFileEntry.file.SetAttribs(EXFAT_ENTRY_ATTRIB_SUBDIR);
		fFileInfoEntry.file_info.SetStartCluster(Cluster());
		fFileInfoEntry.file_info.SetFlag(0);
	}
}


Inode::Inode(Volume* volume)
	:
	fVolume(volume),
	fID(0),
	fCache(NULL),
	fMap(NULL),
	fInitStatus(B_NO_INIT)
{
	_Init();
}


Inode::~Inode()
{
	TRACE("Inode destructor\n");
	file_cache_delete(FileCache());
	file_map_delete(Map());
	TRACE("Inode destructor: Done\n");
}


status_t
Inode::InitCheck()
{
	return fInitStatus;
}


status_t
Inode::UpdateNodeFromDisk()
{
	DirectoryIterator iterator(this);
	iterator.LookupEntry(this);
	return B_OK;
}


cluster_t
Inode::NextCluster(cluster_t cluster) const
{
	if (!IsContiguous() || IsDirectory())
		return GetVolume()->NextCluster(cluster);
	return cluster + 1;
}


mode_t
Inode::Mode() const
{
	mode_t mode = S_IRUSR | S_IRGRP | S_IROTH;
	if (!fVolume->IsReadOnly())
		mode |= S_IWUSR | S_IWGRP | S_IWOTH;
	if (fFileEntry.file.Attribs() & EXFAT_ENTRY_ATTRIB_SUBDIR)
		mode |= S_IFDIR | S_IXUSR | S_IXGRP | S_IXOTH;
	else
		mode |= S_IFREG;
	return mode;
}


status_t
Inode::CheckPermissions(int accessMode) const
{
	// you never have write access to a read-only volume
	if ((accessMode & W_OK) != 0 && fVolume->IsReadOnly())
		return B_READ_ONLY_DEVICE;

	return check_access_permissions(accessMode, Mode(), (gid_t)GroupID(),
		(uid_t)UserID());
}


status_t
Inode::FindBlock(off_t pos, off_t& physical, off_t *_length)
{
	DataStream stream(fVolume, this, Size());
	return stream.FindBlock(pos, physical, _length);
}


status_t
Inode::ReadAt(off_t pos, uint8* buffer, size_t* _length)
{
	return file_cache_read(FileCache(), NULL, pos, buffer, _length);
}


bool
Inode::VisitFile(struct exfat_entry* entry)
{
	fFileEntry = *entry;
	return false;
}


bool
Inode::VisitFileInfo(struct exfat_entry* entry)
{
	fFileInfoEntry = *entry;
	return false;
}


void
Inode::_Init()
{
	memset(&fFileEntry, 0, sizeof(fFileEntry));
	memset(&fFileInfoEntry, 0, sizeof(fFileInfoEntry));
	rw_lock_init(&fLock, "exfat inode");
}


// If divisible by 4, but not divisible by 100, but divisible by 400, it's a leap year
// 1996 is leap, 1900 is not, 2000 is, 2100 is not
#define IS_LEAP_YEAR(y) ((((y) % 4) == 0) && (((y) % 100) || ((((y)) % 400) == 0)))

/* returns leap days since 1970 */
static int leaps(int yr, int mon)
{
	// yr is 1970-based, mon 0-based
	int result = (yr+2)/4 - (yr + 70) / 100;
	if((yr+70) >= 100) result++; // correct for 2000
	if (IS_LEAP_YEAR(yr + 1970))
		if (mon < 2) result--;
	return result;
}

static int daze[] = { 0,0,31,59,90,120,151,181,212,243,273,304,334,0,0,0 };

void
Inode::_GetTimespec(uint16 date, uint16 time, struct timespec &timespec) const
{
	static int32 tzoffset = -1; /* in minutes */
	if (tzoffset == -1)
		tzoffset = get_timezone_offset() / 60;

	time_t days = daze[(date >> 5) & 15] + ((date >> 9) + 10) * 365
		+ leaps((date >> 9) + 10, ((date >> 5) & 15) - 1) + (date & 31) -1;

	timespec.tv_sec = ((days * 24 + (time >> 11)) * 60 + ((time >> 5) & 63)
		- tzoffset) * 60 + 2 * (time & 31);
	timespec.tv_nsec = 0;
}


