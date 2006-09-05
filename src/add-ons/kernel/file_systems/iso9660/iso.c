/*
**		Copyright 1999, Be Incorporated.   All Rights Reserved.
**		This file may be used under the terms of the Be Sample Code License.
**
**		Copyright 2001, pinc Software.  All Rights Reserved.
**
**		iso9960/multi-session
**			2001-03-11: added multi-session support, axeld.
*/

#include <malloc.h>
#include <SupportDefs.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <Drivers.h>
#include <OS.h>
#include <KernelExport.h> 
#include <string.h>
#include <dirent.h>
#include <time.h>
#include <malloc.h>
#include <ByteOrder.h>
#include <fs_cache.h>

#include "rock.h"
#include "iso.h"

//#define TRACE_ISO9660 1
#if TRACE_ISO9660
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


// Just needed here
static status_t unicode_to_utf8(const char	*src, int32	*srcLen, char *dst, int32 *dstLen);


// ISO9660 should start with this string
const char *kISO9660IDString = "CD001";

#if 0
static int   	GetLogicalBlockSize(int fd);
static off_t 	GetNumDeviceBlocks(int fd, int block_size);
#endif
static int   	GetDeviceBlockSize(int fd);

static int		InitVolDate(ISOVolDate *date, char *buf);
static int		InitRecDate(ISORecDate *date, char *buf);
static int		InitVolDesc(nspace *vol, char *buf);

#if 0  // currently unused
static int
GetLogicalBlockSize(int fd)
{
	partition_info p_info;

	if (ioctl(fd, B_GET_PARTITION_INFO, &p_info) == B_NO_ERROR) {
		TRACE(("GetLogicalBlockSize: ioctl suceed\n"));
		return p_info.logical_block_size;
	}
	TRACE(("GetLogicalBlockSize = ioctl returned error\n"));

	return 0;
}


static off_t
GetNumDeviceBlocks(int fd, int block_size)
{
	struct stat			st;
	device_geometry		dg;

	if (ioctl(fd, B_GET_GEOMETRY, &dg) >= 0) {
		return (off_t)dg.cylinder_count *
				(off_t)dg.sectors_per_track *
				(off_t)dg.head_count;
	}

	/* if the ioctl fails, try just stat'ing in case it's a regular file */
	if (fstat(fd, &st) < 0)
		return 0;

	return st.st_size / block_size;
}
#endif

static int
GetDeviceBlockSize(int fd)
{
	struct stat     st;
	device_geometry dg;

	if (ioctl(fd, B_GET_GEOMETRY, &dg) < 0)  {
		if (fstat(fd, &st) < 0 || S_ISDIR(st.st_mode))
			return 0;

		return 512;   /* just assume it's a plain old file or something */
	}

	return dg.bytes_per_sector;
}


// From EncodingComversions.cpp

// Pierre's (modified) Uber Macro

// NOTE: iso9660 seems to store the unicode text in big-endian form
#define u_to_utf8(str, uni_str)\
{\
	if ((B_BENDIAN_TO_HOST_INT16(uni_str[0])&0xff80) == 0)\
		*str++ = B_BENDIAN_TO_HOST_INT16(*uni_str++);\
	else if ((B_BENDIAN_TO_HOST_INT16(uni_str[0])&0xf800) == 0) {\
		str[0] = 0xc0|(B_BENDIAN_TO_HOST_INT16(uni_str[0])>>6);\
		str[1] = 0x80|(B_BENDIAN_TO_HOST_INT16(*uni_str++)&0x3f);\
		str += 2;\
	} else if ((B_BENDIAN_TO_HOST_INT16(uni_str[0])&0xfc00) != 0xd800) {\
		str[0] = 0xe0|(B_BENDIAN_TO_HOST_INT16(uni_str[0])>>12);\
		str[1] = 0x80|((B_BENDIAN_TO_HOST_INT16(uni_str[0])>>6)&0x3f);\
		str[2] = 0x80|(B_BENDIAN_TO_HOST_INT16(*uni_str++)&0x3f);\
		str += 3;\
	} else {\
		int   val;\
		val = ((B_BENDIAN_TO_HOST_INT16(uni_str[0])-0xd7c0)<<10) | (B_BENDIAN_TO_HOST_INT16(uni_str[1])&0x3ff);\
		str[0] = 0xf0 | (val>>18);\
		str[1] = 0x80 | ((val>>12)&0x3f);\
		str[2] = 0x80 | ((val>>6)&0x3f);\
		str[3] = 0x80 | (val&0x3f);\
		uni_str += 2; str += 4;\
	}\
}	


static status_t
unicode_to_utf8(const char	*src, int32	*srcLen, char *dst, int32 *dstLen)
{
	int32 srcLimit = *srcLen;
	int32 dstLimit = *dstLen;
	int32 srcCount = 0;
	int32 dstCount = 0;
	
	for (srcCount = 0; srcCount < srcLimit; srcCount += 2) {
		uint16  *UNICODE = (uint16 *)&src[srcCount];
		uchar	utf8[4];
		uchar	*UTF8 = utf8;
		int32 utf8Len;
		int32 j;

		u_to_utf8(UTF8, UNICODE);

		utf8Len = UTF8 - utf8;
		if ((dstCount + utf8Len) > dstLimit)
			break;

		for (j = 0; j < utf8Len; j++)
			dst[dstCount + j] = utf8[j];
		dstCount += utf8Len;
	}

	*srcLen = srcCount;
	*dstLen = dstCount;
	
	return ((dstCount > 0) ? B_NO_ERROR : B_ERROR);
}


//	#pragma mark -
// Functions specific to iso driver.


int
ISOMount(const char *path, const int flags, nspace **newVol, bool allow_joliet)
{
	// path: 		path to device (eg, /dev/disk/scsi/030/raw)
	// partition:	partition number on device ????
	// flags:		currently unused

	// determine if it is an ISO volume.
	char buffer[ISO_PVD_SIZE];
	bool done = false;
	bool is_iso = false;
	off_t offset = 0x8000;
	ssize_t retval;
	partition_info partitionInfo;
	int deviceBlockSize, multiplier;
	nspace *vol;
	int result = B_NO_ERROR;

	(void)flags;

	TRACE(("ISOMount - ENTER\n"));

	vol = (nspace *)calloc(sizeof(nspace), 1);
	if (vol == NULL) {
		TRACE(("ISOMount - mem error \n"));
		return ENOMEM;
	}

	memset(&partitionInfo, 0, sizeof(partition_info));

	/* open and lock the device */
	vol->fdOfSession = open(path, O_RDONLY);

	/* try to open the raw device to get access to the other sessions as well */
	if (vol->fdOfSession >= 0) {
		if (ioctl(vol->fdOfSession, B_GET_PARTITION_INFO, &partitionInfo) < B_NO_ERROR) {
			TRACE(("B_GET_PARTITION_INFO: ioctl returned error\n"));
			strcpy(partitionInfo.device, path);
		}
		TRACE(("ISOMount: open device/file \"%s\"\n", partitionInfo.device));

		vol->fd = open(partitionInfo.device, O_RDONLY);
	}

	if (vol->fdOfSession < 0 || vol->fd < 0) {
		close(vol->fd);
		close(vol->fdOfSession);

		TRACE(("ISO9660 ERROR - Unable to open <%s>\n", path));
		free(vol);
		return EINVAL;
	}

	deviceBlockSize = GetDeviceBlockSize(vol->fdOfSession);
	if (deviceBlockSize < 0)  {
		TRACE(("ISO9660 ERROR - device block size is 0\n"));
		close(vol->fd);
		close(vol->fdOfSession);

		free(vol);
		return EINVAL;
	}

	vol->joliet_level = 0;
	while ((!done) && (offset < 0x10000)) {
		retval = read_pos (vol->fdOfSession, offset, (void *)buffer, ISO_PVD_SIZE);
		if (retval < ISO_PVD_SIZE) {
			is_iso = false;
			break;
		}

		if (strncmp(buffer + 1, kISO9660IDString, 5) == 0) {
			if ((*buffer == 0x01) && (!is_iso)) {
				// ISO_VD_PRIMARY

				off_t maxBlocks;

				TRACE(("ISOMount: Is an ISO9660 volume, initting rec\n"));
				
				InitVolDesc(vol, buffer);
				strncpy(vol->devicePath,path,127);
				vol->id = ISO_ROOTNODE_ID;
				TRACE(("ISO9660: vol->blockSize = %d\n", vol->logicalBlkSize[FS_DATA_FORMAT]));

				multiplier = deviceBlockSize / vol->logicalBlkSize[FS_DATA_FORMAT];
				TRACE(("ISOMount: block size multiplier is %d\n", multiplier));

				// if the session is on a real device, size != 0
				if (partitionInfo.size != 0)
					maxBlocks = (partitionInfo.size+partitionInfo.offset) / vol->logicalBlkSize[FS_DATA_FORMAT];
				else
					maxBlocks = vol->volSpaceSize[FS_DATA_FORMAT];

				/* Initialize access to the cache so that we can do cached i/o */
				TRACE(("ISO9660: cache init: dev %d, max blocks %Ld\n", vol->fd, maxBlocks));
				vol->fBlockCache = block_cache_create(vol->fd, maxBlocks,
					vol->logicalBlkSize[FS_DATA_FORMAT], true);
				is_iso = true;
			} else if ((*buffer == 0x02) && is_iso && allow_joliet) {
				// ISO_VD_SUPPLEMENTARY

				// JOLIET extension
				// test escape sequence for level of UCS-2 characterset
			    if (buffer[88] == 0x25 && buffer[89] == 0x2f) {
					switch (buffer[90]) {
						case 0x40: vol->joliet_level = 1; break;
						case 0x43: vol->joliet_level = 2; break;
						case 0x45: vol->joliet_level = 3; break;
					}

					TRACE(("ISO9660 Extensions: Microsoft Joliet Level %d\n", vol->joliet_level));

					// Because Joliet-stuff starts at other sector,
					// update root directory record.
					if (vol->joliet_level > 0)
						InitNode(&(vol->rootDirRec), &buffer[156], NULL, 0);
				}						
			} else if (*(unsigned char *)buffer == 0xff) {
				// ISO_VD_END
				done = true;
			} else
				TRACE(("found header %d\n",*buffer));
		}
		offset += 0x800;
	}

	if (!is_iso) {
		// It isn't an ISO disk.
		if (vol->fdOfSession >= 0)
			close(vol->fdOfSession);
		if (vol->fd >= 0)
			close(vol->fd);

		free(vol);
		vol = NULL;
		result = EINVAL;
		TRACE(("ISOMount: Not an ISO9660 volume!\n"));
	}

	TRACE(("ISOMount - EXIT, result %s, returning %p\n", strerror(result), vol));
	*newVol = vol;
	return result;
}


/**	Reads in a single directory entry and fills in the values in the
 *	dirent struct. Uses the cookie to keep track of the current block
 *	and position within the block. Also uses the cookie to determine when
 *	it has reached the end of the directory file.
 *
 *	NOTE: If your file sytem seems to work ok from the command line, but
 *	the tracker doesn't seem to like it, check what you do here closely;
 *	in particular, if the d_ino in the stat struct isn't correct, the tracker
 *	will not display the entry.
 */

int
ISOReadDirEnt(nspace *ns, dircookie *cookie, struct dirent *dirent, size_t bufsize)
{
	off_t totalRead = cookie->pos + ((cookie->block - cookie->startBlock) *
			ns->logicalBlkSize[FS_DATA_FORMAT]);
	off_t cacheBlock;
	char *blockData;
	int	result = B_NO_ERROR;
	int bytesRead = 0;
	bool block_out = false;

	TRACE(("ISOReadDirEnt - ENTER\n"));	

	// If we're at the end of the data in a block, move to the next block.	
	while (1) {
		blockData = (char *)block_cache_get_etc(ns->fBlockCache, cookie->block, 0,
							ns->logicalBlkSize[FS_DATA_FORMAT]);
		block_out = true;

		if (blockData != NULL && *(blockData + cookie->pos) == 0) {
			//NULL data, move to next block.
			block_cache_put(ns->fBlockCache, cookie->block);
			block_out = false;
			totalRead += ns->logicalBlkSize[FS_DATA_FORMAT] - cookie->pos;
			cookie->pos = 0;
			cookie->block++;
		}
		else
			break;

		if (totalRead >= cookie->totalSize)
			break;
	}

	cacheBlock = cookie->block;
	if (blockData != NULL && totalRead < cookie->totalSize) {
		vnode node;

		if ((result = InitNode(&node, blockData + cookie->pos, &bytesRead, ns->joliet_level)) == 
						B_NO_ERROR)
		{
			int nameBufSize = (bufsize - (2 * sizeof(dev_t) + 2* sizeof(ino_t) +
					sizeof(unsigned short)));

			dirent->d_ino = (cookie->block << 30) + (cookie->pos & 0xFFFFFFFF);
			dirent->d_reclen = node.fileIDLen;

			if (node.fileIDLen <= nameBufSize) {
				// need to do some size checking here.
				strncpy(dirent->d_name, node.fileIDString, node.fileIDLen +1);
				TRACE(("ISOReadDirEnt  - success, name is %s\n", dirent->d_name));
			} else {
				TRACE(("ISOReadDirEnt - ERROR, name %s does not fit in buffer of size %d\n", node.fileIDString, nameBufSize));
				result = EINVAL;
			}
			cookie->pos += bytesRead;
		}
	} else  {
		if (totalRead >= cookie->totalSize)
			result = ENOENT;
		else
			result = ENOMEM;
	}

	if (block_out)
		block_cache_put(ns->fBlockCache, cacheBlock);

	TRACE(("ISOReadDirEnt - EXIT, result is %s, vnid is %Lu\n", strerror(result), dirent->d_ino));
	return result;
}


int
InitVolDesc(nspace *vol, char *buffer)
{
	TRACE(("InitVolDesc - ENTER\n"));

	vol->volDescType = *(uint8 *)buffer++;

	vol->stdIDString[5] = '\0';
	strncpy(vol->stdIDString, buffer, 5);
	buffer += 5;

	vol->volDescVersion = *(uint8 *)buffer;
	buffer += 2; // 8th byte unused

	vol->systemIDString[32] = '\0';
	strncpy(vol->systemIDString, buffer, 32);
	buffer += 32;
	TRACE(("InitVolDesc - system id string is %s\n", vol->systemIDString));

	vol->volIDString[32] = '\0';
	strncpy(vol->volIDString, buffer, 32);
	buffer += (32 + 80-73 + 1);	// bytes 80-73 unused
	TRACE(("InitVolDesc - volume id string is %s\n", vol->volIDString));

	vol->volSpaceSize[LSB_DATA] = *(uint32 *)buffer;
	buffer += 4;
	vol->volSpaceSize[MSB_DATA] = *(uint32 *)buffer;
	buffer+= (4 + 120-89 + 1); 		// bytes 120-89 unused

	vol->volSetSize[LSB_DATA] = *(uint16*)buffer;
	buffer += 2;
	vol->volSetSize[MSB_DATA] = *(uint16*)buffer;
	buffer += 2;

	vol->volSeqNum[LSB_DATA] = *(uint16*)buffer;
	buffer += 2;
	vol->volSeqNum[MSB_DATA] = *(uint16*)buffer;
	buffer += 2;

	vol->logicalBlkSize[LSB_DATA] = *(uint16*)buffer;
	buffer += 2;
	vol->logicalBlkSize[MSB_DATA] = *(uint16*)buffer;
	buffer += 2;

	vol->pathTblSize[LSB_DATA] = *(uint32*)buffer;
	buffer += 4;
	vol->pathTblSize[MSB_DATA] = *(uint32*)buffer;
	buffer += 4;

	vol->lPathTblLoc[LSB_DATA] = *(uint16*)buffer;
	buffer += 2;
	vol->lPathTblLoc[MSB_DATA] = *(uint16*)buffer;
	buffer += 2;

	vol->optLPathTblLoc[LSB_DATA] = *(uint16*)buffer;
	buffer += 2;
	vol->optLPathTblLoc[MSB_DATA] = *(uint16*)buffer;
	buffer += 2;

	vol->mPathTblLoc[LSB_DATA] = *(uint16*)buffer;
	buffer += 2;
	vol->mPathTblLoc[MSB_DATA] = *(uint16*)buffer;
	buffer += 2;

	vol->optMPathTblLoc[LSB_DATA] = *(uint16*)buffer;
	buffer += 2;
	vol->optMPathTblLoc[MSB_DATA] = *(uint16*)buffer;
	buffer += 2;

	// Fill in directory record.
	InitNode(&(vol->rootDirRec), buffer, NULL, 0);

	vol->rootDirRec.id = ISO_ROOTNODE_ID;
	buffer += 34;

	vol->volSetIDString[128] = '\0';
	strncpy(vol->volSetIDString, buffer, 128);
	buffer += 128;
	TRACE(("InitVolDesc - volume set id string is %s\n", vol->volSetIDString));

	vol->pubIDString[128] = '\0';
	strncpy(vol->pubIDString, buffer, 128);
	buffer += 128;
	TRACE(("InitVolDesc - volume pub id string is %s\n", vol->pubIDString));

	vol->dataPreparer[128] = '\0';
	strncpy(vol->dataPreparer, buffer, 128);
	buffer += 128;
	TRACE(("InitVolDesc - volume dataPreparer string is %s\n", vol->dataPreparer));

	vol->appIDString[128] = '\0';
	strncpy(vol->appIDString, buffer, 128);
	buffer += 128;
	TRACE(("InitVolDesc - volume app id string is %s\n", vol->appIDString));

	vol->copyright[38] = '\0';
	strncpy(vol->copyright, buffer, 38);
	buffer += 38;
	TRACE(("InitVolDesc - copyright is %s\n", vol->copyright));

	vol->abstractFName[38] = '\0';
	strncpy(vol->abstractFName, buffer, 38);
	buffer += 38;

	vol->biblioFName[38] = '\0';
	strncpy(vol->biblioFName, buffer, 38);
	buffer += 38;

	InitVolDate(&(vol->createDate), buffer);
	buffer += 17;

	InitVolDate(&(vol->modDate), buffer);
	buffer += 17;

	InitVolDate(&(vol->expireDate), buffer);
	buffer += 17;

	InitVolDate(&(vol->effectiveDate), buffer);
	buffer += 17;

	vol->fileStructVers = *(uint8 *)buffer;
	return 0;
}


int
InitNode(vnode *rec, char *buffer, int *_bytesRead, uint8 joliet_level)
{
	int 	result = B_NO_ERROR;
	uint8 	recLen = *(uint8 *)buffer++;
	bool    no_rock_ridge_stat_struct = TRUE;

	if (_bytesRead != NULL)
		*_bytesRead = recLen;

	TRACE(("InitNode - ENTER, bufstart is %p, record length is %d bytes\n", buffer, recLen));

	rec->cache = NULL;

	if (recLen > 0) {
		rec->extAttrRecLen = *(uint8 *)buffer++;

		rec->startLBN[LSB_DATA] = *(uint32 *)buffer;
		buffer += 4;
		rec->startLBN[MSB_DATA] = *(uint32 *)buffer;
		buffer += 4;
		TRACE(("InitNode - data start LBN is %ld\n", rec->startLBN[FS_DATA_FORMAT]));

		rec->dataLen[LSB_DATA] = *(uint32 *)buffer;
		buffer += 4;
		rec->dataLen[MSB_DATA] = *(uint32 *)buffer;
		buffer += 4;
		TRACE(("InitNode - data length is %ld\n", rec->dataLen[FS_DATA_FORMAT]));

		InitRecDate(&(rec->recordDate), buffer);
		buffer += 7;

		rec->flags = *(uint8 *)buffer;
		buffer++;
		TRACE(("InitNode - flags are %d\n", rec->flags));

		rec->fileUnitSize = *(uint8 *)buffer;
		buffer++;
		TRACE(("InitNode - fileUnitSize is %d\n", rec->fileUnitSize));

		rec->interleaveGapSize = *(uint8 *)buffer;
		buffer++;
		TRACE(("InitNode - interleave gap size = %d\n", rec->interleaveGapSize));

		rec->volSeqNum = *(uint32 *)buffer;
		buffer += 4;
		TRACE(("InitNode - volume seq num is %ld\n", rec->volSeqNum));

		rec->fileIDLen = *(uint8*)buffer;
		buffer++;
		TRACE(("InitNode - file id length is %d\n", rec->fileIDLen));

		if (rec->fileIDLen > 0) {
			// JOLIET extension:
			// on joliet discs, buffer[0] can be 0 for Unicoded filenames,
			// so I've added a check here to test explicitely for
			// directories (which have length 1)
			if (rec->fileIDLen == 1) {
				// Take care of "." and "..", the first two dirents are
				// these in iso.
				if (buffer[0] == 0) {
					rec->fileIDString = strdup(".");
					rec->fileIDLen = 1;
				} else if (buffer[0] == 1) {
					rec->fileIDString = strdup("..");
					rec->fileIDLen = 2;
				}
			} else {
				// JOLIET extension:
				// convert Unicode16 string to UTF8
				if (joliet_level > 0) {
					// Assume that the unicode->utf8 conversion produces 4 byte
					// utf8 characters, and allocate that much space
					rec->fileIDString = (char *)malloc(rec->fileIDLen * 2 + 1);
					if (rec->fileIDString != NULL) {
						status_t err;
						int32 srcLen = rec->fileIDLen;
						int32 dstLen = rec->fileIDLen * 2;

						err = unicode_to_utf8(buffer, &srcLen, rec->fileIDString, &dstLen);
						if (err < B_NO_ERROR) {
							dprintf("iso9660: error converting unicode->utf8\n");
							result = err;
						} else {
							rec->fileIDString[dstLen] = '\0';
							rec->fileIDLen = dstLen;	 						
						}
					} else {
						// Error
						result = ENOMEM;
						TRACE(("InitNode - unable to allocate memory!\n"));
					}
				} else {
					rec->fileIDString = (char *)malloc((rec->fileIDLen) + 1);

					if (rec->fileIDString != NULL) {	
						strncpy(rec->fileIDString, buffer, rec->fileIDLen);
						rec->fileIDString[rec->fileIDLen] = '\0';
					} else {
						// Error
						result = ENOMEM;
						TRACE(("InitNode - unable to allocate memory!\n"));
					}
				}
			}
						
			// Get rid of semicolons, which are used to delineate file versions.q
			{
				char *semi = NULL;
				while ((semi = strchr(rec->fileIDString, ';')) != NULL)
					semi[0] = '\0';
			}
			TRACE(("DirRec ID String is: %s\n", rec->fileIDString));

			if (result == B_NO_ERROR) {
				buffer += rec->fileIDLen;
				if (!(rec->fileIDLen % 2))
					buffer++;
	
				// Now we're at the start of the rock ridge stuff
				{
					char *altName = NULL;
					char *slName = NULL;
					uint16 altNameSize = 0;
					uint16 slNameSize = 0;
					uint8 slFlags = 0;
					uint8 length = 0;
					bool done = FALSE;

					TRACE(("RR: Start of extensions, but at %p\n", buffer));
					
					memset(&(rec->attr.stat), 0, 2 * sizeof(struct stat));
					
					// Set defaults, in case there is no RR stuff.
					rec->attr.stat[FS_DATA_FORMAT].st_mode = (S_IRUSR | S_IRGRP | S_IROTH);
					
					while (!done)
					{
						buffer += length;
						length = *(uint8 *)(buffer + 2);
						switch (0x100 * buffer[0] + buffer[1])
						{
							// Stat structure stuff
							case 'PX':
							{
								uint8 bytePos = 3;
								TRACE(("RR: found PX, length %u\n", length));
								rec->attr.pxVer = *(uint8*)(buffer + bytePos++);
                                no_rock_ridge_stat_struct = FALSE;
								
								// st_mode
								rec->attr.stat[LSB_DATA].st_mode = *(mode_t*)(buffer + bytePos);
								bytePos += 4;
								rec->attr.stat[MSB_DATA].st_mode = *(mode_t*)(buffer + bytePos);
								bytePos += 4;
								
								// st_nlink
								rec->attr.stat[LSB_DATA].st_nlink = *(nlink_t*)(buffer+bytePos);
								bytePos += 4;
								rec->attr.stat[MSB_DATA].st_nlink = *(nlink_t*)(buffer + bytePos);
								bytePos += 4;
								
								// st_uid
								rec->attr.stat[LSB_DATA].st_uid = *(uid_t*)(buffer+bytePos);
								bytePos += 4;
								rec->attr.stat[MSB_DATA].st_uid = *(uid_t*)(buffer+bytePos);
								bytePos += 4;
								
								// st_gid
								rec->attr.stat[LSB_DATA].st_gid = *(gid_t*)(buffer+bytePos);
								bytePos += 4;
								rec->attr.stat[MSB_DATA].st_gid = *(gid_t*)(buffer+bytePos);
								bytePos += 4;
								break;
							}	
							
							case 'PN':
								TRACE(("RR: found PN, length %u\n", length));
								break;
							
							// Symbolic link info
							case 'SL':
							{
								uint8	bytePos = 3;
								uint8	lastCompFlag = 0;
								uint8	addPos = 0;
								bool	slDone = FALSE;
								bool	useSeparator = TRUE;
								
								TRACE(("RR: found SL, length %u\n", length));
								TRACE(("Buffer is at %p\n", buffer));
								TRACE(("Current length is %u\n", slNameSize));
								//kernel_debugger("");
								rec->attr.slVer = *(uint8*)(buffer + bytePos++);
								slFlags = *(uint8*)(buffer + bytePos++);
								
								TRACE(("sl flags are %u\n", slFlags));
								while (!slDone && bytePos < length)
								{
									uint8 compFlag = *(uint8*)(buffer + bytePos++);
									uint8 compLen = *(uint8*)(buffer + bytePos++);
									
									if (slName == NULL) useSeparator = FALSE;
									
									addPos = slNameSize;
									
									TRACE(("sl comp flags are %u, length is %u\n", compFlag, compLen));
									TRACE(("Current name size is %u\n", slNameSize));
									switch (compFlag)
									{
										case SLCP_CONTINUE:
											useSeparator = FALSE;
										default:
											// Add the component to the total path.
											slNameSize += compLen;
											if ( useSeparator ) slNameSize++;
											if (slName == NULL) 
												slName = (char*)malloc(slNameSize + 1);
											else
												slName = (char*)realloc(slName, slNameSize + 1);
											
											if (useSeparator) 
											{
												TRACE(("Adding separator\n"));
												slName[addPos++] = '/';
											}	
											
											TRACE(("doing memcopy of %u bytes at offset %d\n", compLen, addPos));
											memcpy((slName + addPos), (buffer + bytePos), compLen);
											
											addPos += compLen;
											useSeparator = TRUE;
										break;
										
										case SLCP_CURRENT:
											TRACE(("InitNode - found link to current directory\n"));
											slNameSize += 2;
											if (slName == NULL) 
												slName = (char*)malloc(slNameSize + 1);
											else
												slName = (char*)realloc(slName, slNameSize + 1);
											memcpy(slName + addPos, "./", 2);
											useSeparator = FALSE;
										break;
										
										case SLCP_PARENT:
											slNameSize += 3;
											if (slName == NULL) 
												slName = (char*)malloc(slNameSize + 1);
											else
												slName = (char*)realloc(slName, slNameSize + 1);
											memcpy(slName + addPos, "../", 3);
											useSeparator = FALSE;
										break;
										
										case SLCP_ROOT:
											TRACE(("InitNode - found link to root directory\n"));
											slNameSize += 1;
											if (slName == NULL) 
												slName = (char*)malloc(slNameSize + 1);
											else
												slName = (char*)realloc(slName, slNameSize + 1);
											memcpy(slName + addPos, "/", 1);
											useSeparator = FALSE;
										break;
										
										case SLCP_VOLROOT:
											slDone = TRUE;
										break;
										
										case SLCP_HOST:
											slDone = TRUE;
										break;
									}
									slName[slNameSize] = '\0';
									lastCompFlag = compFlag;
									bytePos += compLen;
									TRACE(("Current sl name is \'%s\'\n", slName));
								}
								rec->attr.slName = slName;
								TRACE(("InitNode = symlink name is \'%s\'\n", slName));
								break;
							}

							// Altername name
							case 'NM':
							{
								uint8	bytePos = 3;
								uint8	flags = 0;
								uint16	oldEnd = altNameSize;

								altNameSize += length - 5;
								if (altName == NULL)
									altName = (char *)malloc(altNameSize + 1);
								else
									altName = (char *)realloc(altName, altNameSize + 1);

								TRACE(("RR: found NM, length %u\n", length));
								// Read flag and version.
								rec->attr.nmVer = *(uint8 *)(buffer + bytePos++);
								flags = *(uint8 *)(buffer + bytePos++);
							
								TRACE(("RR: nm buffer is %s, start at %p\n", (buffer + bytePos), buffer + bytePos));
	
								// Build the file name.
								memcpy(altName + oldEnd, buffer + bytePos, length - 5);
								altName[altNameSize] = '\0';
								TRACE(("RR: alt name is %s\n", altName));
								
								// If the name is not continued in another record, update
								// the record name.
								if (!(flags & NM_CONTINUE))
								{
									// Get rid of the ISO name, replace with RR name.
									if (rec->fileIDString != NULL)
										free(rec->fileIDString);
									rec->fileIDString = altName;
									rec->fileIDLen = altNameSize;
								}
								break;
							}

							// Deep directory record masquerading as a file.
							case 'CL':							
								TRACE(("RR: found CL, length %u\n", length));
								rec->flags |= ISO_ISDIR;
								rec->startLBN[LSB_DATA] = *(uint32*)(buffer+4);
								rec->startLBN[MSB_DATA] = *(uint32*)(buffer+8);
								break;

							case 'PL':
								TRACE(("RR: found PL, length %u\n", length));
								break;

							// Relocated directory, we should skip.
							case 'RE':
								result = EINVAL;
								TRACE(("RR: found RE, length %u\n", length));
								break;

							case 'TF':
								TRACE(("RR: found TF, length %u\n", length));
								break;

							case 'RR':
								TRACE(("RR: found RR, length %u\n", length));
								break;

							default:
								TRACE(("RR: %c%c (%d, %d)\n",buffer[0],buffer[1],buffer[0],buffer[1]));
								TRACE(("RR: End of extensions.\n"));
								done = TRUE;
								break;
						}
					}
				}
			}
		} else {
			TRACE(("InitNode - File ID String is 0 length\n"));
			result = ENOENT;
		}
	} else
		result = ENOENT;

	TRACE(("InitNode - EXIT, result is %s name is \'%s\'\n", strerror(result), rec->fileIDString));	

	if (no_rock_ridge_stat_struct) {
		if (rec->flags & ISO_ISDIR)
			rec->attr.stat[FS_DATA_FORMAT].st_mode |= (S_IFDIR|S_IXUSR|S_IXGRP|S_IXOTH);
		else
			rec->attr.stat[FS_DATA_FORMAT].st_mode |= (S_IFREG);
	}

	return result;
}


static int
InitVolDate(ISOVolDate *date, char *buffer)
{
	memcpy(date, buffer, 17);
	return 0;
}


static int
InitRecDate(ISORecDate *date, char *buffer)
{
	memcpy(date, buffer, 7);
	return 0;
}


int
ConvertRecDate(ISORecDate* inDate, time_t* outDate)
{
	time_t	time;
	int		days, i, year, tz;
	
	year = inDate->year -70;
	tz = inDate->offsetGMT;
	
	if (year < 0) {
		time = 0;
	} else {
		const int monlen[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

		days = (year * 365);

		if (year > 2)
			days += (year + 1)/ 4;

		for (i = 1; (i < inDate->month) && (i < 12); i++) {
			days += monlen[i-1];
		}

		if (((year + 2) % 4) == 0 && inDate->month > 2)
			days++;

		days += inDate->date - 1;
		time = ((((days*24) + inDate->hour) * 60 + inDate->minute) * 60)
					+ inDate->second;
		if (tz & 0x80)
			tz |= (-1 << 8);	

		if (-48 <= tz && tz <= 52)
			time += tz *15 * 60;
	}
	*outDate = time;
	return 0;
}

