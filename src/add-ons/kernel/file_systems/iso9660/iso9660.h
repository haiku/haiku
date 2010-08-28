/*
 * Copyright 1999, Be Incorporated.   All Rights Reserved.
 * This file may be used under the terms of the Be Sample Code License.
 *
 * Copyright 2001, pinc Software.  All Rights Reserved.
 */
#ifndef ISO_9660_H
#define ISO_9660_H


#if FS_SHELL
#	include "fssh_api_wrapper.h"
#else
#	include <stdio.h>
#	include <sys/types.h>
#	include <time.h>
#	include <endian.h>

#	include <fs_interface.h>
#	include <SupportDefs.h>

#	include <lock.h>
#endif


// Size of primary volume descriptor for ISO9660
#define ISO_PVD_SIZE 882

// ISO structure has both msb and lsb first data. These let you do a
// compile-time switch for different platforms.

enum {
	LSB_DATA = 0,
	MSB_DATA
};

// This defines the data format for the platform we are compiling
// for.
#if BYTE_ORDER == __LITTLE_ENDIAN
#	define FS_DATA_FORMAT LSB_DATA
#else
#	define FS_DATA_FORMAT MSB_DATA
#endif

// ISO pdf file spec I read appears to be WRONG about the date format. See
// www.alumni.caltech.edu/~pje/iso9660.html, definition there seems
// to match.
typedef struct ISOVolDate {
 	char	year[5];
 	char	month[3];
 	char	day[3];
 	char	hour[3];
 	char	minute[3];
 	char	second[3];
 	char	hunSec[3];
 	int8	offsetGMT;
} ISOVolDate;

typedef struct ISORecDate {
	uint8	year;			// Year - 1900
	uint8	month;
	uint8	date;
	uint8	hour;
	uint8	minute;
	uint8	second;
	int8	offsetGMT;
} ISORecDate;

// This next section is data structure to hold the data found in the
// rock ridge extensions.
struct rock_ridge_attributes {
	char*			slName;
	struct stat		stat[2];
	uint8			nmVer;
	uint8			pxVer;
	uint8			slVer;
};

struct iso9660_volume;

struct iso9660_inode {
	/* Most drivers will probably want the first things defined here. */
	ino_t		id;
	ino_t	 	parID;		// parent vnode ID.
	struct iso9660_volume* volume;
	void		*cache;		// for file cache

	// End of members other drivers will definitely want.

	/* The rest of this struct is very ISO-specific. You should replace the rest of this
		definition with the stuff your file system needs.
	*/
	uint8		extAttrRecLen;			// Length of extended attribute record.
	uint32		startLBN[2];			// Logical block # of start of file/directory data
	uint32		dataLen[2];				// Length of file section in bytes
	ISORecDate	recordDate;				// Date file was recorded.

										// BIT MEANING
										// --- -----------------------------
	uint8		flags;					// 0 - is hidden
										// 1 - is directory
										// 2 - is "Associated File"
										// 3 - Info is structed according to extended attr record
										// 4 - Owner, group, ppermisssions are specified in the
										//		extended attr record
										// 5 - Reserved
										// 6 - Reserved
										// 7 - File has more that one directory record

	uint8		fileUnitSize;			// Interleave only
	uint8		interleaveGapSize;		// Interleave only
	uint32		volSeqNum;				// Volume sequence number of volume
	char*		name;
	uint32		name_length;

	// The rest is Rock Ridge extensions. I suggest www.leo.org for spec info.
	rock_ridge_attributes attr;
};

// These go with the flags member of the iso9660_volume struct.
enum {
	ISO_IS_HIDDEN				= 0x01,
	ISO_IS_DIR					= 0x02,
	ISO_IS_ASSOCIATED_FILE		= 0x04,
	ISO_EXTENDED_ATTRIBUTE		= 0x08,
	ISO_EXTENDED_PERMISSIONS	= 0x10,
	ISO_MORE_DIRS				= 0x80
};


// Arbitrarily - selected root vnode id
#define ISO_ROOTNODE_ID 1

/* Structure used for directory "cookie". When you are asked
 	to open a directory, you are asked to create a cookie for it
 	and pass it back. The cookie should contain the information you
 	need to determine where you are at in reading the directory
 	entries, incremented every time readdir is called, until finally
 	the end is reached, when readdir returns NULL. */
typedef struct dircookie {
	off_t startBlock;
	off_t block;			// Current block
	off_t pos;				// Position within block.
	off_t totalSize;		// Size of directory file
	off_t id;
} dircookie;

/* You may also need to define a cookie for files, which again is
	allocated every time a file is opened and freed when the free
	cookie function is called. For ISO, we didn't need one.
*/

typedef struct attrfilemap {
	char  *name;
	off_t offset;
} attrfilemap;

// This is the global volume iso9660_volume struct.
struct iso9660_volume {
	// Start of members other drivers will definitely want.
	fs_volume		*volume;			// volume passed fo fs_mount
	dev_t			id;
	int				fd;				// File descriptor
	int				fdOfSession;	// File descriptor of the (mounted) session
	//unsigned int 	blockSize;  	// usually need this, but it's part of ISO
	void                            *fBlockCache;

	char			devicePath[127];
	//off_t			numBlocks;		// May need this, but it's part of ISO

	// End of members other drivers will definitely want.

	// attribute extensions
	int32			readAttributes;
	attrfilemap 	*attrFileMap;
	ino_t			attributesID;

	// JOLIET extension: joliet_level for this volume
	uint8			joliet_level;

	// All this stuff comes straight from ISO primary volume
	// descriptor structure.
	uint8			volDescType;		// Volume Descriptor type								byte1
	char			stdIDString[6];		// Standard ID, 1 extra for null						byte2-6
	uint8			volDescVersion;		// Volume Descriptor version							byte7
																			// 8th byte unused
	char			systemIDString[33];	// System ID, 1 extra for null							byte9-40
	char			volIDString[33];	// Volume ID, 1 extra for null							byte41-72
																			// bytes 73-80 unused
	uint32			volSpaceSize[2];	// #logical blocks, lsb and msb									byte81-88
																		// bytes 89-120 unused
	uint16			volSetSize[2];		// Assigned Volume Set Size of Vol						byte121-124
	uint16			volSeqNum[2];		// Ordinal number of volume in Set						byte125-128
	uint16			logicalBlkSize[2];	// Logical blocksize, usually 2048						byte129-132
	uint32			pathTblSize[2];		// Path table size										byte133-149
	uint16			lPathTblLoc[2];		// Loc (Logical block #) of "Type L" path table		byte141-144
	uint16			optLPathTblLoc[2];	// Loc (Logical block #) of optional Type L path tbl		byte145-148
	uint16			mPathTblLoc[2];		// Loc (Logical block #) of "Type M" path table		byte149-152
	uint16			optMPathTblLoc[2];		// Loc (Logical block #) of optional Type M path tbl	byte153-156
	iso9660_inode			rootDirRec;			// Directory record for root directory					byte157-190
	char			volSetIDString[129]; // Name of multi-volume set where vol is member		byte191-318
	char			pubIDString[129];	// Name of publisher									byte319-446
	char			dataPreparer[129];	// Name of data preparer								byte447-574
	char			appIDString[129];	// Identifies data fomrat								byte575-702
	char			copyright[38];		// Copyright string										byte703-739
	char			abstractFName[38];	// Name of file in root that has abstract				byte740-776
	char			biblioFName[38];	// Name of file in root that has biblio				byte777-813

	ISOVolDate		createDate;			// Creation date										byte
	ISOVolDate		modDate;			// Modification date
	ISOVolDate		expireDate;			// Data expiration date
	ISOVolDate		effectiveDate;		// Data effective data

	uint8			fileStructVers;		// File structure version								byte882
};


status_t ISOMount(const char *path, uint32 flags, iso9660_volume** _newVolume,
	bool allowJoliet);
status_t ISOReadDirEnt(iso9660_volume* ns, dircookie* cookie,
	struct dirent* buffer, size_t bufferSize);
status_t InitNode(iso9660_volume* volume, iso9660_inode* rec, char* buf,
	size_t* bytesRead, bool relocated = false);
status_t ConvertRecDate(ISORecDate* inDate, time_t* outDate);

#endif	/* ISO_9660_H */
