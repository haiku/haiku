/*
**	Copyright 1999, Be Incorporated.   All Rights Reserved.
**	This file may be used under the terms of the Be Sample Code License.
**
**	Copyright 2001, pinc Software.  All Rights Reserved.
*/

#ifndef _ISO_H
#define _ISO_H

#include <stdio.h>
#include <sys/types.h>
#include <fsproto.h>
#include <lock.h>
#include <time.h>
#include <endian.h>

#include <SupportDefs.h>

// ISO structure has both msb and lsb first data. These let you do a 
// compile-time switch for different platforms.

enum
{
	LSB_DATA = 0,
	MSB_DATA
};

// This defines the data format for the platform we are compiling
// for.
#if BYTE_ORDER == __LITTLE_ENDIAN
#define FS_DATA_FORMAT LSB_DATA
#else
#define FS_DATA_FORMAT MSB_DATA
#endif

// ISO pdf file spec I read appears to be WRONG about the date format. See
// www.alumni.caltech.edu/~pje/iso9660.html, definition there seems
// to match.
typedef struct ISOVolDate
{
 	char	year[5];
 	char	month[3];
 	char	day[3];
 	char	hour[3];
 	char	minute[3];
 	char	second[3];
 	char	hunSec[3];
 	int8	offsetGMT;
} ISOVolDate;

typedef struct ISORecDate
{
	uint8	year;			// Year - 1900
	uint8	month;
	uint8	date;
	uint8	hour;
	uint8	minute;
	uint8	second;
	int8	offsetGMT;
} ISORecDate;

/* This next section is data structure to hold the data found in the rock ridge extensions. */
typedef struct RRAttr
{
	char*			slName;
	struct stat		stat[2];
	uint8			nmVer;
	uint8			pxVer;
	uint8			slVer;
} RRAttr;

/* For each item on the disk (directory, file, etc), your filesystem should allocate a vnode struct and 
	pass it back to the kernel when fs_read_vnode is called. This struct is then passed back in to 
	your file system by functions that reference an item on the disk. You'll need to be able to
	create a vnode from a vnode id, either by hashing the id number or encoding the information needed
	to create the vnode in the vnode id itself. Vnode ids are assigned by your file system when the
	filesystem walk function is called. For this driver, the block number is encoded in the upper bits
	of the vnode id, and the offset within the block in the lower, allowing me to just read the info
	to fill in the vnode struct from the disk. When the kernel is done with a vnode, it will call 
	fs_write_vnode (somewhat of a misnomer) where you should deallocate the struct.
*/

typedef struct vnode
{
	/* Most drivers will probably want the first things defined here. */
	vnode_id	id; 
	vnode_id 	parID;		// parent vnode ID.
	
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
	uint8		fileIDLen;				// Length of volume "ID" (name)
	char*		fileIDString;			// Volume "ID" (name)

	// The rest is Rock Ridge extensions. I suggest www.leo.org for spec info.
	RRAttr		attr;
} vnode;

// These go with the flags member of the nspace struct.
enum
{
	ISO_ISHIDDEN = 0,
	ISO_ISDIR		= 2,
	ISO_ISASSOCFILE = 4,
	ISO_EXTATTR = 8,
	ISO_EXTPERM = 16,
	ISO_MOREDIRS = 128
};


// Arbitrarily - selected root vnode id
#define ISO_ROOTNODE_ID 1

/* Structure used for directory "cookie". When you are asked
 	to open a directory, you are asked to create a cookie for it
 	and pass it back. The cookie should contain the information you
 	need to determine where you are at in reading the directory 
 	entries, incremented every time readdir is called, until finally
 	the end is reached, when readdir returns NULL. */
typedef struct dircookie
{
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

typedef struct attrfilemap
{
	char  *name;
	off_t offset;
} attrfilemap;

/* This is the global volume nspace struct. When mount is called , this struct should
	be allocated. It is passed back into the functions so that you can get at any
	global information you need. You'll need to change this struct to suit your purposes.
*/

typedef struct nspace
{
	// Start of members other drivers will definitely want.
	nspace_id		id;				// ID passed in to fs_mount
	int				fd;				// File descriptor
	int				fdOfSession;	// File descriptor of the (mounted) session
	//unsigned int 	blockSize;  	// usually need this, but it's part of ISO
		
	char			devicePath[127];
	//off_t			numBlocks;		// May need this, but it's part of ISO
	
	// End of members other drivers will definitely want.

	// attribute extensions
	int32			readAttributes;
	attrfilemap 	*attrFileMap;
	vnode_id		attributesID;

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
	vnode			rootDirRec;			// Directory record for root directory					byte157-190
	char			volSetIDString[29];	// Name of multi-volume set where vol is member		byte191-318
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
} nspace;

int 		ISOMount(const char *path, const int flags, nspace** newVol, bool allow_joliet);
int			ISOReadDirEnt(nspace* ns, dircookie* cookie, struct dirent* buf, size_t bufsize);
int			InitNode(vnode* rec, char* buf, int* bytesRead, uint8 joliet_level);
int			ConvertRecDate(ISORecDate* inDate, time_t* outDate);

#endif
